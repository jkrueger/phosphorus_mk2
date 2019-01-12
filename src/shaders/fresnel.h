
float fresnel_dielectric(float cosi, float eta)
{
  float c = fabs(cosi);
  float g = eta * eta - 1 + c * c;
  float result;

  if (g > 0) {
    g = sqrt(g);
    float A = (g - c) / (g + c);
    float B = (c * (g + c) - 1) / (c * (g - c) + 1);
    result = 0.5 * A * A * (1 + B * B);
  }
  else {
    result = 1.0;
  }

  return result;
}

color fresnel_conductor(float cosi, color eta, color k)
{
  return color(0,0,0);
}
