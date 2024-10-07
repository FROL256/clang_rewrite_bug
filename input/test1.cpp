// complex numbers adapted from PBRT-v4
struct complex 
{
  inline complex() : re(0), im(0) {}
  inline complex(float re_) : re(re_), im(0) {}
  inline complex(float re_, float im_) : re(re_), im(im_) {}

  inline complex operator-() const { return {-re, -im}; }
  inline complex operator+(complex z) const { return {re + z.re, im + z.im}; }
  inline complex operator-(complex z) const { return {re - z.re, im - z.im}; }
  inline complex operator*(complex z) const { return {re * z.re - im * z.im, re * z.im + im * z.re}; }

  inline complex operator/(complex z) const 
  {
      float scale = 1 / (z.re * z.re + z.im * z.im);
      return {scale * (re * z.re + im * z.im), scale * (im * z.re - re * z.im)};
  }

  inline friend complex operator+(float value, complex z) { return complex(value) + z; }
  inline friend complex operator-(float value, complex z) { return complex(value) - z; }
  inline friend complex operator*(float value, complex z) { return complex(value) * z; }
  inline friend complex operator/(float value, complex z) { return complex(value) / z; }

  float re, im;
};

inline static float real(const complex &z) { return z.re; }
inline static float imag(const complex &z) { return z.im; }
inline static float complex_norm(const complex &z) { return z.re * z.re + z.im * z.im; }

void foo(int* a, int *b) {
  if (a[0] > 1) {
    b[0] = 2;
  }
  else
    b[0] = 3;
}

void bar(float x, float y); // just a declaration
