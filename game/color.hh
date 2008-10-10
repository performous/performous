#ifndef PERFORMOUS_COLOR_HH
#define PERFORMOUS_COLOR_HH

struct Color {
	double r;
	double g;
	double b;
	double a;
	Color(double red = 0.0, double grn = 0.0, double blu = 0.0, double alp = 1.0): r(red), g(grn), b(blu), a(alp) {}
};

#endif

