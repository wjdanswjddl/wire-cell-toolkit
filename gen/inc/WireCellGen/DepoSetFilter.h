//  Module is designed to filter depos based on provided APA dimentions.
//  It takes pointer to full stack of depos from DepoFanout and outputs
//  a poonter to only ones contained in a given volume

#ifndef WIRECELLGEN_DEPOSETFILTER
#define WIRECELLGEN_DEPOSETFILTER

#include "WireCellIface/IDepoSetFilter.h"
#include "WireCellIface/INamed.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellAux/Logger.h"
#include "WireCellUtil/BoundingBox.h"

namespace WireCell::Gen {

    class DepoSetFilter : public Aux::Logger, public IDepoSetFilter, public IConfigurable {
       public:
        DepoSetFilter();
        virtual ~DepoSetFilter();

        // IDepoSetFilter
        virtual bool operator()(const input_pointer& in, output_pointer& out);

        /// WireCell::IConfigurable interface.
        virtual void configure(const WireCell::Configuration& config);
        virtual WireCell::Configuration default_configuration() const;

       private:
        std::vector<WireCell::BoundingBox> m_boxes;
        std::size_t m_count{0};
    };

}  // namespace WireCell::Gen

#endif
