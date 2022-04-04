// Add noise by group to the waveform
// mailto:smartynen@bnl.gov

#ifndef WIRECELL_GEN_ADDGROUPNOISE
#define WIRECELL_GEN_ADDGROUPNOISE

#include "WireCellAux/Logger.h"
#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IDFT.h"
#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IRandom.h"
#include "WireCellUtil/Waveform.h"

#include <string>

namespace WireCell {
namespace Gen {

class AddGroupNoise : Aux::Logger, public IFrameFilter, public IConfigurable {
public:
  AddGroupNoise(const std::string &model = "", const std::string &map = "",
                const std::string &rng = "Random");

  virtual ~AddGroupNoise();

  /// IFrameFilter
  virtual bool operator()(const input_pointer &inframe,
                          output_pointer &outframe);

  /// IConfigurable
  virtual void configure(const WireCell::Configuration &config);
  virtual WireCell::Configuration default_configuration() const;

  size_t argclosest(std::vector<float> const &vec, float value);

private:
  IRandom::pointer m_rng;

  std::string m_spectra_file, m_map_file, m_rng_tn;
  double m_spec_scale;  
  int m_nsamples;
  IDFT::pointer m_dft;
  std::unordered_map<int, int> m_ch2grp;
  std::unordered_map<int, std::vector<float>> m_grp2spec;
  std::unordered_map<int, Waveform::compseq_t> m_grp2noise;

  Log::logptr_t log;
};
} // namespace Gen
} // namespace WireCell

#endif
