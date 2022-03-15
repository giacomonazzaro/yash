syms t o d p h r0 r1;
syms o1 o2 o3;
syms d1 d2 d3;
o = [o1 o2 o3];
d = [d1 d2 d3];
p = o + d * t;
expression = p*p.' - p(3) * p(3) - ((p(3) / h) * r0 + (1 - p(3) / h) * r1)^2
c = coeffs(expression, t)

delta = -c(2)^2 - 4 * c(3) * c(1)
% simplfy(delta)

