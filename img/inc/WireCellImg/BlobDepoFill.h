/**
   BlobDepoFill produces a new blobset with blob charge filled
   from depo charge.

   Except for replacing blob charge all input blob information is
   retained in the output.

   The depos are best to be post-drift.  They may span multiple
   AnodePlanes in the transverse direction and will be filtered in the
   same way as DepoTransform.  The faces refereced by the blobs are
   used to perform this filtering.

 */

#ifndef WIRECELLIMG_BLOBDEPOFILL
#define WIRECELLIMG_BLOBDEPOFILL

#include "WireCellAux/Logger.h"

#include "WireCellIface/IBlobDepoFill.h"
#include "WireCellIface/IConfigurable.h"

#include <functional>

namespace WireCell::Img {

    class BlobDepoFill : public Aux::Logger,
                         public IBlobDepoFill,
                         public IConfigurable
    {
      public:

        BlobDepoFill();
        virtual ~BlobDepoFill();

        // IConfigurable
        virtual void configure(const WireCell::Configuration& cfg);
        virtual WireCell::Configuration default_configuration() const;

        // IBlobDepoFill.  Input tuple is (blob,depo).
        virtual bool operator()(const input_tuple_type& intup,
                                output_pointer& out);

      private:
        double m_toffset{0};
        double m_nsigma{3.0};
        double m_speed{0};      // must configure
        int m_pindex{2};        // primary plane index
        size_t m_count{0};
    };

}

#endif

