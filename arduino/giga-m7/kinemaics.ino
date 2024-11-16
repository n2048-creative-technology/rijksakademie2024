


void invKin(float coordinates[], float angles[]) {
  for (int i = 0; i < NUM_JOINTS; i++) {
    if(isValidFloat(coordinates[i])) angles[i] = coordinates[i];
  }
}

float magSq(float x[]) {
    return (x[0] * x[0])+(x[1] * x[1])+(x[2] * x[2]);
}

void calcServos(float coordinates[], float servoPositions[]) {

    float x = x;
    float y = y;
    float z = z;
    float roll = radians(roll);
    float pitch = radians(pitch);
    float yaw = radians(yaw);

  // Define the base and platform attachment points
  // You may need to adjust these to match your Stewart platform's geometry
   float baseJoint[6][3] = {
      {-212.098, -7.25, 0}, 
      {-99.6, 1187.25, 500}, 
      {100.311, 186.501, 500}, 
      {212.098, -6.05, 500}, 
      {112, -180, 500}, 
      {-112, -180, 500}
  };
 float platformJoint[6][3] = {
      {-200.342, 0, 100}, 
      {-100.171, 173.501, 100}, 
      {100.171, 173.501, 100}, 
      {200.342, 0, 100}, 
      {100.171, -173.501, 100}, 
      {-100.171, -173.501, 100}
  };

  float q[6][3]= {{0.}};
  float l[6][3]= {{0.}};
  float A[6][3]= {{0.}};
  float initialHeight = 500;

  float platformRadius = 20;
  float baseRadius = 15;
  
  float hornLength = 120;
  float legLength = 500;
  float beta[6] = {-8*PI/3, PI/3, 0, -PI, -4*PI/3, -7*PI/3};
  
  //new angles new small platform, need to invert the servo horns 
  float baseAngles[6] = { 314.9, 345.1, 74.9, 105.1, 194.9, 225.1 };
  float platformAngles[6]  = { 322.9, 337.1, 82.9, 97.1, 202.9, 217.1};

  for (int i=0; i<6; i++) {
      baseJoint[i][0] = baseRadius*cos(radians(baseAngles[i]));
      baseJoint[i][1] = baseRadius*sin(radians(baseAngles[i]));
      baseJoint[i][2] = 0.;
  }

  for (int i=0; i<6; i++) {
    platformJoint[i][0]= platformRadius*cos(radians(platformAngles[i]));
    platformJoint[i][1] = platformRadius*sin(radians(platformAngles[i]));
    platformJoint[i][2] = 0.;
  }


//calcQ
  for (int i=0; i<6; i++) {
    // rotation
    q[i][0] = cos(yaw)*cos(pitch)*platformJoint[i][0] +
      (-sin(yaw)*cos(roll)+cos(yaw)*sin(pitch)*sin(roll))*platformJoint[i][1] +
      (sin(yaw)*sin(roll)+cos(yaw)*sin(pitch)*cos(roll))*platformJoint[i][2];

    q[i][1] = sin(yaw)*cos(pitch)*platformJoint[i][0] +
      (cos(yaw)*cos(roll)+sin(yaw)*sin(pitch)*sin(roll))*platformJoint[i][1] +
      (-cos(yaw)*sin(roll)+sin(yaw)*sin(pitch)*cos(roll))*platformJoint[i][2];

    q[i][2] = -sin(pitch)*platformJoint[i][0] +
      cos(pitch)*sin(roll)*platformJoint[i][1] +
      cos(pitch)*cos(roll)*platformJoint[i][2];

    // translation
    for(int j=0;j<3;j++){
      q[i][j] += coordinates[j] +  initialHeight;
      l[i][j] = q[i][j] - baseJoint[i][j];
    }
  }

//calcAlpha
  for (int i=0; i<6; i++) {
    float L = magSq(l[i])-(legLength*legLength)+(hornLength*hornLength);
    float M = 2*hornLength*(q[i][2]-baseJoint[i][2]);
    float N = 2*hornLength*(cos(beta[i])*(q[i][0]-baseJoint[i][0]) + sin(beta[i])*(q[i][1]-baseJoint[i][1]));
    servoPositions[i] = asin(L/sqrt(M*M+N*N)) - atan2(N, M);
  }
}






