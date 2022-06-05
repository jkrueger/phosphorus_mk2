
color rgb_ramp_extrapolate(color lut[], int size, float at) {
  float f = at;
  color t0, dy;

  if (f < 0.0) {
    t0 = lut[0];
    dy = t0 - lut[1];
    f  = -f;
  }
  else {
    t0 = lut[size - 1];
    dy = t0 - lut[size - 2];
    f  = f - 1,0;
  }

  return t0 + dy * f * (size - 1);
}

color rgb_ramp_lut(
    color lut[]
  , float at
  , int interpolate
  , int extrapolate)
{
  int size = arraylength(lut);

  float f = at;

  if ((f < 0.0 || f > 1.0) && extrapolate) {
    return rgb_ramp_extrapolate(lut, size, f);
  }

  f = clamp(at, 0.0, 1.0) * (size - 1);

  int   i = (int) f;
  float t = f - (float) i;

  color result = lut[i];

  if (interpolate && t > 0.0) {
    result = (1.0 - t) * result + t * lut[i + 1];
  }

  return result;
}
