/* ==========================================================================
   VISUALISATION D'ORIENTATION DE FUSÉE — Processing
   ==========================================================================
   Lit un CSV exporté depuis Excel (temps, ax, ay, az, gx, gy, gz),
   fusionne accéléro + gyro avec un filtre complémentaire en quaternions,
   et anime un modèle 3D simple de la fusée.

   FORMAT ATTENDU DU CSV (data/imu_data.csv), avec en-têtes :
     time, ax, ay, az, gx, gy, gz
   - time : en secondes (temps écoulé, pas forcément un pas constant)
   - ax, ay, az : accélération en g (ou m/s^2, voir NORMALISE_ACCEL plus bas)
   - gx, gy, gz : vitesse angulaire en rad/s (si tes données sont en deg/s,
     mets GYRO_EN_DEGRES = true)
   ========================================================================== */

import processing.core.PMatrix3D;

// ---------- Réglages à adapter ----------
boolean GYRO_EN_DEGRES   = false;   // true si gx,gy,gz sont en °/s dans le CSV
boolean NORMALISE_ACCEL  = false;   // true si ax,ay,az sont en g (norme ~1 au repos)
float   ALPHA_COMPL      = 0.98;   // poids de base du gyro au repos (0.9-0.995)

// Détection de poussée : au-delà de ce taux d'écart par rapport à la norme
// au repos, on réduit la confiance en l'accéléro ; au-delà du seuil haut,
// on l'ignore complètement (100% gyro). Entre les deux, transition douce.
float SEUIL_DEVIATION_BAS  = 0.3;  // 15% d'écart -> on commence à réduire l'accéléro
float SEUIL_DEVIATION_HAUT = 0.9;  // 50% d'écart -> 100% gyro
float ACCEL_REST_MAGNITUDE = 1.0;   // recalculée au démarrage à partir de la 1ère ligne
float   VITESSE_LECTURE  = 1;    // 1.0 = temps réel, 2.0 = 2x plus vite, etc.

// ---------- Données ----------
Table data;
int idx = 0;
float tStart;          // instant de démarrage de la lecture (millis())
float tDataStart;      // "time" de la première ligne du CSV

// ---------- Orientation ----------
Quaternion orientation = new Quaternion(1, 0, 0, 0); // identité au départ
Quaternion refCorrection = new Quaternion(1, 0, 0, 0); // calibrée au démarrage (voir setup)

// ---------- Caméra ----------
// Vue en 3/4 avec une légère élévation, pour bien percevoir l'inclinaison
// de la fusée (par ex. proche de la verticale au décollage) plutôt qu'une
// vue de face qui l'écraserait visuellement. À ajuster selon le rendu :
// - CAM_AZIMUT_DEG   : tourne autour de la fusée (0 = de face)
// - CAM_ELEVATION_DEG: hauteur du point de vue (0 = à hauteur d'yeux, 90 = du dessus)
// - CAM_DISTANCE      : recule/rapproche la caméra
float CAM_AZIMUT_DEG     = 35;
float CAM_ELEVATION_DEG  = 20;
float CAM_DISTANCE       = 480;

void setup() {
  size(900, 700, P3D);
  data = loadCSVRobuste("imu_data.csv");
  tDataStart = data.getRow(0).getFloat("time");
  tStart = millis();

  // ---- Calibration au démarrage : on force l'orientation de la 1ère ligne
  // du CSV à s'afficher exactement verticale (nez vers le haut), et on
  // conserve ce même décalage fixe pour toute la suite. Comme il n'y a pas
  // de magnétomètre, le cap absolu (lacet) est de toute façon arbitraire ;
  // ça permet aussi d'absorber un éventuel désalignement du capteur au
  // montage sur le roll/pitch initial.
  TableRow row0 = data.getRow(0);
  PVector accel0 = remapAccel(row0.getFloat("ax"), row0.getFloat("ay"), row0.getFloat("az"));
  ACCEL_REST_MAGNITUDE = accel0.mag(); // norme de l'accéléro au repos, sur le pas de tir
  Quaternion orientation0 = orientationDepuisAccel(accel0.x, accel0.y, accel0.z);
  Quaternion vertical = fromAxisAngle(new PVector(1, 0, 0), -HALF_PI); // nez vers le haut à l'écran
  refCorrection = vertical.multiply(orientation0.conjugate()).normalized();
}

/* --------------------------------------------------------------------------
   CHARGEMENT ROBUSTE DU CSV
   Un export Excel en français utilise typiquement ";" comme séparateur de
   colonnes et "," comme séparateur décimal (au lieu de "," et "." attendus
   par Processing), ce qui produit des NaN avec loadTable() standard.
   Cette fonction détecte le séparateur et normalise les décimales avant
   de construire un Table "propre" en mémoire.
   -------------------------------------------------------------------------- */
Table loadCSVRobuste(String filename) {
  String[] lignes = loadStrings(filename);
  if (lignes == null || lignes.length < 2) {
    println("ERREUR : impossible de lire " + filename + " (fichier vide ou introuvable)");
    exit();
  }

  // Détection du séparateur à partir de la ligne d'en-tête
  String separateur = lignes[0].contains(";") ? ";" : ",";

  Table t = new Table();
  String[] entetes = split(lignes[0], separateur);
  for (int i = 0; i < entetes.length; i++) {
    t.addColumn(trim(entetes[i]));
  }

  for (int i = 1; i < lignes.length; i++) {
    if (trim(lignes[i]).length() == 0) continue; // ignorer les lignes vides
    String[] champs = split(lignes[i], separateur);
    TableRow row = t.addRow();
    for (int c = 0; c < entetes.length && c < champs.length; c++) {
      String valeurBrute = trim(champs[c]);
      // Normaliser la virgule décimale française en point
      valeurBrute = valeurBrute.replace(",", ".");
      row.setFloat(c, float(valeurBrute));
    }
  }

  println("CSV chargé : " + t.getRowCount() + " lignes, séparateur détecté = '" + separateur + "'");
  return t;
}

void draw() {
  background(15);
  lights();
  ambientLight(80, 80, 80);
  directionalLight(255, 255, 255, 0.3, 0.5, -1);

  // ---- Avancer dans le CSV en fonction du temps écoulé ----
  float tElapsed = (millis() - tStart) / 1000.0 * VITESSE_LECTURE;
  float tCible = tDataStart + tElapsed;
  while (idx < data.getRowCount() - 1 && data.getRow(idx + 1).getFloat("time") < tCible) {
    idx++;
  }
  if (idx >= data.getRowCount() - 1) {
    idx = 0;
    tStart = millis();
    tDataStart = data.getRow(0).getFloat("time");
  }

  TableRow row = data.getRow(idx);
  TableRow rowPrev = data.getRow(max(idx - 1, 0));
  float dt = row.getFloat("time") - rowPrev.getFloat("time");
  if (dt <= 0) dt = 1.0 / 100.0; // valeur de secours si deux temps identiques

  float ax = row.getFloat("ax");
  float ay = row.getFloat("ay");
  float az = row.getFloat("az");
  float gx = row.getFloat("gx");
  float gy = row.getFloat("gy");
  float gz = row.getFloat("gz");

  if (GYRO_EN_DEGRES) {
    gx = radians(gx);
    gy = radians(gy);
    gz = radians(gz);
  }

  // ---- Remappage des axes du capteur vers le repère du modèle 3D ----
  PVector accelB = remapAccel(ax, ay, az);
  PVector gyroB  = remapGyro(gx, gy, gz);

  mettreAJourOrientation(accelB.x, accelB.y, accelB.z, gyroB.x, gyroB.y, gyroB.z, dt);

  // ---- Caméra en 3/4, vue depuis un point élevé et décalé sur le côté ----
  float az_ = radians(CAM_AZIMUT_DEG);
  float el_ = radians(CAM_ELEVATION_DEG);
  float eyeX = width / 2.0  + CAM_DISTANCE * cos(el_) * sin(az_);
  float eyeY = height / 2.0 - CAM_DISTANCE * sin(el_);   // Y négatif = plus haut à l'écran
  float eyeZ = CAM_DISTANCE * cos(el_) * cos(az_);
  camera(eyeX, eyeY, eyeZ,
         width / 2.0, height / 2.0, 0,
         0, 1, 0);

  // ---- Rendu ----
  pushMatrix();
  translate(width / 2.0, height / 2.0, 0);
  scale(1, -1, 1); // pour un repère "Z vers le haut" plus intuitif

  applyMatrix(refCorrection.multiply(orientation).normalized().toRotationMatrix());
  drawRocket();
  popMatrix();

  drawHUD(ax, ay, az, gx, gy, gz);
}

/* --------------------------------------------------------------------------
   REMAPPAGE DES AXES DU CAPTEUR VERS LE REPÈRE DU MODÈLE 3D
   Le modèle est construit avec Z = axe long de la fusée (nez vers +Z).
   Sur ce montage, l'axe X du capteur pointe vers la queue (donc -X = vers
   le nez), et Y/Z du capteur sont les deux axes latéraux. Le signe sur
   l'axe Y du modèle est nécessaire pour conserver un repère direct
   (sinon les rotations calculées partiraient dans le mauvais sens).
   -------------------------------------------------------------------------- */
PVector remapAccel(float ax, float ay, float az) {
  return new PVector(ay, -az, -ax);
}

PVector remapGyro(float gx, float gy, float gz) {
  return new PVector(gy, -gz, -gx);
}

/* --------------------------------------------------------------------------
   ORIENTATION (ROLL/PITCH UNIQUEMENT) DÉDUITE DE L'ACCÉLÉRO SEUL
   -------------------------------------------------------------------------- */
Quaternion orientationDepuisAccel(float axB, float ayB, float azB) {
  PVector accel = new PVector(axB, ayB, azB);
  if (NORMALISE_ACCEL) accel.normalize();
  float roll  = atan2(accel.y, accel.z);
  float pitch = atan2(-accel.x, sqrt(accel.y * accel.y + accel.z * accel.z));
  return fromEuler(roll, pitch, 0);
}

/* --------------------------------------------------------------------------
   FILTRE COMPLÉMENTAIRE EN QUATERNIONS
   1) On intègre le gyro (rotation relative fluide mais qui dérive)
   2) On calcule l'inclinaison "absolue" donnée par l'accéléro (bruitée,
      mais sans dérive, et incapable de donner le yaw)
   3) On mélange les deux : ALPHA_COMPL vers le gyro, (1-ALPHA_COMPL) vers l'accéléro
   -------------------------------------------------------------------------- */
void mettreAJourOrientation(float ax, float ay, float az,
                             float gx, float gy, float gz, float dt) {
  // 1) Intégration gyroscopique : petite rotation delta sur ce pas de temps
  PVector axeRotation = new PVector(gx, gy, gz);
  float vitesseAngulaire = axeRotation.mag();
  Quaternion deltaQ;
  if (vitesseAngulaire > 0.0001) {
    axeRotation.normalize();
    deltaQ = fromAxisAngle(axeRotation, vitesseAngulaire * dt);
  } else {
    deltaQ = new Quaternion(1, 0, 0, 0);
  }
  Quaternion orientationGyro = orientation.multiply(deltaQ).normalized();

  // 2) Orientation "absolue" déduite de l'accéléro (roll & pitch uniquement)
  Quaternion orientationAccel = orientationDepuisAccel(ax, ay, az);

  // 3) Gain adaptatif : si la norme de l'accéléro s'écarte de sa valeur au
  //    repos (poussée du moteur, vibrations fortes...), on réduit
  //    progressivement la confiance qu'on lui accorde, jusqu'à l'ignorer
  //    complètement (100% gyro) au-delà de SEUIL_DEVIATION_HAUT.
  float magnitude = sqrt(ax * ax + ay * ay + az * az);
  float deviation = abs(magnitude / ACCEL_REST_MAGNITUDE - 1.0);
  float t = constrain(map(deviation, SEUIL_DEVIATION_BAS, SEUIL_DEVIATION_HAUT, 0, 1), 0, 1);
  float alphaAdaptatif = lerp(ALPHA_COMPL, 1.0, t);

  // 4) Fusion (nlerp : interpolation linéaire + renormalisation ; suffisant
  //    pour de la visu, contrairement à un vrai slerp c'est approximatif
  //    mais largement stable pour ce cas d'usage)
  orientation = nlerp(orientationAccel, orientationGyro, alphaAdaptatif);
}

/* --------------------------------------------------------------------------
   MODÈLE 3D SIMPLE DE LA FUSÉE (à remplacer par un .obj si tu en as un)
   -------------------------------------------------------------------------- */
void drawRocket() {
  noStroke();

  // Corps cylindrique (axe Z, aligné avec le nez et les ailerons)
  fill(200);
  pushMatrix();
  drawCylinder(20, 160, 24);
  popMatrix();

  // Cône (nez), pointant vers +Z (l'axe "avant" de la fusée)
  fill(220, 60, 60);
  pushMatrix();
  translate(0, 0, 80);
  drawCone(20, 60, 24);
  popMatrix();

  // Ailerons (repères visuels pour voir le roulis) — plaques fines dans le
  // sens tangentiel, qui dépassent largement dans le sens radial
  fill(80, 80, 220);
  for (int i = 0; i < 3; i++) {
    pushMatrix();
    rotateZ(TWO_PI / 3 * i);
    translate(35, 0, -70); // 35 = surface du cylindre (20) + moitié de la longueur radiale (15)
    box(30, 4, 40);        // 30 = longueur radiale, 4 = épaisseur tangentielle, 40 = le long de l'axe
    popMatrix();
  }
}

void drawCylinder(float r, float h, int detail) {
  float angleStep = TWO_PI / detail;
  beginShape(QUAD_STRIP);
  for (int i = 0; i <= detail; i++) {
    float a = i * angleStep;
    float x = cos(a) * r, y = sin(a) * r;
    vertex(x, y, -h / 2);
    vertex(x, y, h / 2);
  }
  endShape();
}

void drawCone(float r, float h, int detail) {
  float angleStep = TWO_PI / detail;
  beginShape(TRIANGLE_FAN);
  vertex(0, 0, h);
  for (int i = 0; i <= detail; i++) {
    float a = i * angleStep;
    vertex(cos(a) * r, sin(a) * r, 0);
  }
  endShape();
}

void drawHUD(float ax, float ay, float az, float gx, float gy, float gz) {
  camera(); // repasse en 2D par-dessus la 3D
  hint(DISABLE_DEPTH_TEST);
  fill(255);
  textSize(14);
  text("t = " + nf(data.getRow(idx).getFloat("time"), 1, 2) + " s", 20, 30);
  text("accel (g)   : " + nf(ax,1,2) + ", " + nf(ay,1,2) + ", " + nf(az,1,2), 20, 50);
  text("gyro (rad/s): " + nf(gx,1,2) + ", " + nf(gy,1,2) + ", " + nf(gz,1,2), 20, 70);
  hint(ENABLE_DEPTH_TEST);
}

/* ==========================================================================
   CLASSE QUATERNION (w, x, y, z) — pas de classe native dans Processing
   ========================================================================== */
// Fonctions "constructeurs" placées en dehors de la classe : dans un sketch
// Processing, Quaternion est une classe interne du sketch et Java interdit
// les méthodes static dans une classe interne non-statique.
Quaternion fromAxisAngle(PVector axis, float angle) {
  float half = angle / 2.0;
  float s = sin(half);
  return new Quaternion(cos(half), axis.x * s, axis.y * s, axis.z * s);
}

// Convention: roll = rotation autour de X, pitch = autour de Y, yaw = autour de Z
Quaternion fromEuler(float roll, float pitch, float yaw) {
  float cr = cos(roll * 0.5),  sr = sin(roll * 0.5);
  float cp = cos(pitch * 0.5), sp = sin(pitch * 0.5);
  float cy = cos(yaw * 0.5),   sy = sin(yaw * 0.5);
  return new Quaternion(
    cr * cp * cy + sr * sp * sy,
    sr * cp * cy - cr * sp * sy,
    cr * sp * cy + sr * cp * sy,
    cr * cp * sy - sr * sp * cy
  );
}

// Interpolation linéaire normalisée entre deux quaternions (a -> b), t dans [0,1]
Quaternion nlerp(Quaternion a, Quaternion b, float t) {
  float dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
  Quaternion bb = b;
  if (dot < 0) bb = new Quaternion(-b.w, -b.x, -b.y, -b.z);
  Quaternion result = new Quaternion(
    lerp(a.w, bb.w, t),
    lerp(a.x, bb.x, t),
    lerp(a.y, bb.y, t),
    lerp(a.z, bb.z, t)
  );
  return result.normalized();
}

class Quaternion {
  float w, x, y, z;

  Quaternion(float w, float x, float y, float z) {
    this.w = w; this.x = x; this.y = y; this.z = z;
  }

  Quaternion multiply(Quaternion q) {
    return new Quaternion(
      w * q.w - x * q.x - y * q.y - z * q.z,
      w * q.x + x * q.w + y * q.z - z * q.y,
      w * q.y - x * q.z + y * q.w + z * q.x,
      w * q.z + x * q.y - y * q.x + z * q.w
    );
  }

  Quaternion conjugate() {
    return new Quaternion(w, -x, -y, -z);
  }

  Quaternion normalized() {
    float m = sqrt(w * w + x * x + y * y + z * z);
    if (m < 0.00001) return new Quaternion(1, 0, 0, 0);
    return new Quaternion(w / m, x / m, y / m, z / m);
  }

  PMatrix3D toRotationMatrix() {
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    return new PMatrix3D(
      1 - 2 * (yy + zz),     2 * (xy - wz),     2 * (xz + wy), 0,
          2 * (xy + wz), 1 - 2 * (xx + zz),     2 * (yz - wx), 0,
          2 * (xz - wy),     2 * (yz + wx), 1 - 2 * (xx + yy), 0,
                      0,                 0,                 0, 1
    );
  }
}
