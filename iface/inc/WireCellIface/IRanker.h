/**
   An IRanker<T> provides a method taking some interface object and
   returning a sclar "rank" value.  Interpreting this value depends on
   implementation.

 */

#ifndef WIRECELLIFACE_IRANKER
#define WIRECELLIFACE_IRANKER

#include "WireCellUtil/IComponent.h"

namespace WireCell {

    template<typename IThing>
    class IRanker : virtual public IComponent<IRanker<IThing>> {
      public:
        virtual ~IRanker() {};

        virtual double rank(const typename IThing::pointer& thing) = 0;
    };

}
#endif
