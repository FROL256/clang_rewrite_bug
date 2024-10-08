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

//float test() 
//{  
//  complex a = complex(1.0f, 2.0f);
//  complex b = complex(3.0f, 4.0f);
//  complex c = a + b/2.0f;
//  complex d = a + b/complex(2.0f);
//  complex e = a + 2.0f/b;
//  complex f = a + 2.0f/complex(b);
//  return c.re + c.im;
//}

complex test_bug(complex cosTheta, complex eta, float lambda)
{
  return cosTheta + eta/lambda;
}

complex test_ok(complex cosTheta, complex eta, float lambda)
{
  return cosTheta + eta/complex(lambda);
}
