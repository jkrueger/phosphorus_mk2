
color rgb_ramp_lut(
    color lut[]
  , float at
  , int interpolate
  , int extrapolate)
{
  float f = at;

  if ((f < 0.0f || f > 1.0f) && extrapolate) {
    return rgb_ramp_extrapolate(lut, f);
  }

  f = clamp(at, 0.0f, 1.0f) * (arraysize(lut) - 1);

  int i = (int) f;
  float t = f - (float) i;

  color result = lut[i];

  if (interpolate) {
    result = lerp(t, result, lut[i + 1]);
  }

  return result;
}
