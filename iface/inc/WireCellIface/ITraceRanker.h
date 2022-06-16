// A ranker of ITraces.

#ifndef WIRECELL_ITRACERANKER
#define WIRECELL_ITRACERANKER

#include "WireCellIface/IRanker.h"
#include "WireCellIface/ITrace.h"

namespace WireCell {

    class ITraceRanker : virtual public IRanker<ITrace> {
      public:
        virtual ~ITraceRanker();

        // provide:
        // virtual double rank(const ITrace::pointer& trace);
        
   };

}
#endif
