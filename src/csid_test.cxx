#include "sys.h"
#include "math/CS.h"
#include "debug.h"

using math::CS;

namespace csid {
  using namespace math::csid;

  DECLARE_CSID(foobar);
} // namespace csid

template<CS cs>
void f()
{
  Dout(dc::notice, "Calling f<" << cs << ">()");
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  Dout(dc::notice, "Entering main()");

  Dout(dc::notice, "pixels = " << csid::pixels);
  Dout(dc::notice, "plot = " << csid::plot);
  Dout(dc::notice, "foobar = " << csid::foobar);

  f<csid::foobar>();
}
