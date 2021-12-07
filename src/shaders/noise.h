
float safe_noise(float p)
{
  float f = noise("noise", p);
  if (isinf(f))
    return 0.5;
  return f;
}

float safe_noise(vector2 p)
{
  float f = noise("noise", p.x, p.y);
  if (isinf(f))
    return 0.5;
  return f;
}

float safe_noise(vector3 p)
{
  float f = noise("noise", p);
  if (isinf(f))
    return 0.5;
  return f;
}

float safe_noise(vector4 p)
{
  float f = noise("noise", vector3(p.x, p.y, p.z), p.w);
  if (isinf(f))
    return 0.5;
  return f;
}

float safe_snoise(float p)
{
  float f = noise("snoise", p);
  if (isinf(f))
    return 0.0;
  return f;
}

float safe_snoise(vector2 p)
{
  float f = noise("snoise", p.x, p.y);
  if (isinf(f))
    return 0.0;
  return f;
}

float safe_snoise(vector3 p)
{
  float f = noise("snoise", p);
  if (isinf(f))
    return 0.0;
  return f;
}

float safe_snoise(vector4 p)
{
  float f = noise("snoise", vector3(p.x, p.y, p.z), p.w);
  if (isinf(f))
    return 0.0;
  return f;
}