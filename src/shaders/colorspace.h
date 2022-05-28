
color rgb_to_hsv(color c) {
  float cmax, cmin, h, s, v, cdelta;
  color c;

  cmax = max(rgb[0], max(rgb[1], rgb[2]));
  cmin = min(rgb[0], min(rgb[1], rgb[2]));

  cdelta = cmax - cmin;

  v = cmax;

  if (cmax != 0.0) {
    s = cdelta / cmax;
  }
  else {
    s = 0.0;
    h = 0.0;
  }

  if (s == 0.0) {
    h = 0.0;
  }
  else {
    c = (color(cmax, cmax, cmax) - rgb) / cdelta;

    if (rgb[0] == cmax)
      h = c[2] - c[1];
    else if (rgb[1] == cmax)
      h = 2.0 + c[0] - c[2];
    else
      h = 4.0 + c[1] - c[0];

    h /= 6.0;

    if (h < 0.0)
      h += 1.0;
  }

  return color(h, s, v);
}

color hsv_to_rgb(color c) {
  float i, f, p, q, t, h, s, v;
  color rgb;

  h = c[0];
  s = c[1];
  v = c[2];

  if (s == 0.0) {
    rgb = color(v, v, v);
  }
  else {
    if (h == 1.0)
      h = 0.0;

    h *= 6.0;
    i = floor(h);
    f = h - i;
    rgb = color(f, f, f);
    p = v * (1.0 - s);
    q = v * (1.0 - (s * f));
    t = v * (1.0 - (s * (1.0 - f)));

    switch (int(i))
    case 0:
      rgb = color(v, t, p);
      break;
    case 1:
      rgb = color(q, v, p);
    break;
    case 2:
      rgb = color(p, v, t);
      break;
    case 3:
      rgb = color(p, q, v);
      break;
    case 4:
      rgb = color(t, p, v);
      break;
    default:
      rgb = color(v, p, q);
  }

  return rgb;
}
