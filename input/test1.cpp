struct complex 
{
  inline complex() : re(0), im(0) {}
  inline complex(float re_) : re(re_), im(0) {}
  inline complex(float re_, float im_) : re(re_), im(im_) {}
  inline complex operator+(complex z) const { return {re + z.re, im + z.im}; }
  inline complex operator/(complex z) const 
  {
      float scale = 1 / (z.re * z.re + z.im * z.im);
      return {scale * (re * z.re + im * z.im), scale * (im * z.re - re * z.im)};
  }
  float re, im;
};

complex test_bug(complex cosTheta, complex eta, float lambda)
{
  return cosTheta + eta/lambda;
}
