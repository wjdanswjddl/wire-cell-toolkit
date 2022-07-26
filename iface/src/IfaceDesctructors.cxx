// Collect most of the interfaces' virtual destructors.
//
// Some details about this file found in:
// https://github.com/WireCell/wire-cell-iface/issues/5
// 
// Please try to keep the block of #includes and of destructors
// alphabetically ordered.  In Emacs, select each individually and:
// M-x sort-lines.

#include "WireCellIface/IAnodeFace.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IApplication.h"
#include "WireCellIface/IBlob.h"
#include "WireCellIface/IBlobSet.h"
#include "WireCellIface/IBlobSetFanin.h"
#include "WireCellIface/IBlobSetFanout.h"
#include "WireCellIface/IBlobSetSink.h"
#include "WireCellIface/IChannelFilter.h"
#include "WireCellIface/IChannelNoiseDatabase.h"
#include "WireCellIface/IChannelResponse.h"
#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellIface/IChannelStatus.h"
#include "WireCellIface/IClusterFanin.h"
#include "WireCellIface/IClusterFilter.h"
#include "WireCellIface/IClusterFramer.h"
#include "WireCellIface/IClusterSink.h"
#include "WireCellIface/IClustering.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IDataFlowGraph.h"
#include "WireCellIface/IDeconvolution.h"
#include "WireCellIface/IDepo.h"
#include "WireCellIface/IDepoCollector.h"
#include "WireCellIface/IDepoFanout.h"
#include "WireCellIface/IDepoFilter.h"
#include "WireCellIface/IDepoFramer.h"
#include "WireCellIface/IDepoMerger.h"
#include "WireCellIface/IDepoSet.h"
#include "WireCellIface/IDepoSetFanin.h"
#include "WireCellIface/IDepoSetFanout.h"
#include "WireCellIface/IDepoSetFilter.h"
#include "WireCellIface/IDepoSetSink.h"
#include "WireCellIface/IDepoSetSource.h"
#include "WireCellIface/IDepoSink.h"
#include "WireCellIface/IDepoSource.h"
#include "WireCellIface/IDiffuser.h"
#include "WireCellIface/IDiffusion.h"
#include "WireCellIface/IDrifter.h"
#include "WireCellIface/IDuctor.h"
#include "WireCellIface/IFaninNode.h"
#include "WireCellIface/IFanoutNode.h"
#include "WireCellIface/IFieldResponse.h"
#include "WireCellIface/IFilterWaveform.h"
#include "WireCellIface/IFrame.h"
#include "WireCellIface/IFrameFanin.h"
#include "WireCellIface/IFrameFanout.h"
#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IFrameJoiner.h"
#include "WireCellIface/IFrameSink.h"
#include "WireCellIface/IFrameSlicer.h"
#include "WireCellIface/IFrameSlices.h"
#include "WireCellIface/IFrameSource.h"
#include "WireCellIface/IFrameSplitter.h"
#include "WireCellIface/IFunctionNode.h"
#include "WireCellIface/IGroupSpectrum.h"
#include "WireCellIface/IHydraNode.h"
#include "WireCellIface/IJoinNode.h"
#include "WireCellIface/IMeasure.h"
#include "WireCellIface/INamed.h"
#include "WireCellIface/INode.h"
#include "WireCellIface/IPlaneImpactResponse.h"
#include "WireCellIface/IPointFieldSink.h"
#include "WireCellIface/IProcessor.h"
#include "WireCellIface/IQueuedoutNode.h"
#include "WireCellIface/IQueuedoutNode.h"
#include "WireCellIface/IRecombinationModel.h"
#include "WireCellIface/IScalarFieldSink.h"
#include "WireCellIface/ISemaphore.h"
#include "WireCellIface/ISequence.h"
#include "WireCellIface/ISinkNode.h"
#include "WireCellIface/ISlice.h"
#include "WireCellIface/ISliceFanout.h"
#include "WireCellIface/ISliceFrame.h"
#include "WireCellIface/ISliceFrameSink.h"
#include "WireCellIface/ISliceStriper.h"
#include "WireCellIface/ISourceNode.h"
#include "WireCellIface/ISplitNode.h"
#include "WireCellIface/IStripe.h"
#include "WireCellIface/IStripeSet.h"
#include "WireCellIface/ITensorForward.h"
#include "WireCellIface/ITensorPacker.h"
#include "WireCellIface/ITensorSetFilter.h"
#include "WireCellIface/ITensorSetUnpacker.h"
#include "WireCellIface/ITerminal.h"
#include "WireCellIface/ITiling.h"
#include "WireCellIface/ITrace.h"
#include "WireCellIface/ITraceRanker.h"
#include "WireCellIface/IWaveform.h"
#include "WireCellIface/IWaveformMap.h"
#include "WireCellIface/IWireGenerator.h"
#include "WireCellIface/IWireParameters.h"
#include "WireCellIface/IWireSchema.h"
#include "WireCellIface/IWireSource.h"
#include "WireCellIface/IWireSummarizer.h"
#include "WireCellIface/IWireSummary.h"

using namespace WireCell;

IAnodeFace::~IAnodeFace() {}
IAnodePlane::~IAnodePlane() {}
IApplication::~IApplication() {}
IBlob::~IBlob() {}
IBlobSet::~IBlobSet() {}
IBlobSetFanin::~IBlobSetFanin() {}
IBlobSetFanout::~IBlobSetFanout() {}
IBlobSetSink::~IBlobSetSink() {}
IChannelFilter::~IChannelFilter() {}
IChannelNoiseDatabase::~IChannelNoiseDatabase() {}
IChannelResponse::~IChannelResponse() {}
IChannelSpectrum::~IChannelSpectrum() {}
IChannelStatus::~IChannelStatus() {}
IClusterFanin::~IClusterFanin() {}
IClusterFilter::~IClusterFilter() {}
IClusterFramer::~IClusterFramer() {}
IClusterSink::~IClusterSink() {}
IClustering::~IClustering() {}
IConfigurable::~IConfigurable() {}
IDataFlowGraph::~IDataFlowGraph() {}
IDeconvolution::~IDeconvolution() {}
IDepo::~IDepo() {}
IDepoCollector::~IDepoCollector() {}
IDepoFanout::~IDepoFanout() {}
IDepoFilter::~IDepoFilter() {}
IDepoFramer::~IDepoFramer() {}
IDepoMerger::~IDepoMerger() {}
IDepoSet::~IDepoSet() {}
IDepoSetFanin::~IDepoSetFanin() {}
IDepoSetFanout::~IDepoSetFanout() {}
IDepoSetFilter::~IDepoSetFilter() {}
IDepoSetSink::~IDepoSetSink() {}
IDepoSetSource::~IDepoSetSource() {}
IDepoSink::~IDepoSink() {}
IDepoSource::~IDepoSource() {}
IDiffuser::~IDiffuser() {}
IDiffusion::~IDiffusion() {}
IDrifter::~IDrifter() {}
IDuctor::~IDuctor() {}
IFaninNodeBase::~IFaninNodeBase() {}
IFanoutNodeBase::~IFanoutNodeBase() {}
IFieldResponse::~IFieldResponse() {}
IFilterWaveform::~IFilterWaveform() {}
IFrame::~IFrame() {}
IFrameFanin::~IFrameFanin() {}
IFrameFanout::~IFrameFanout() {}
IFrameFilter::~IFrameFilter() {}
IFrameJoiner::~IFrameJoiner() {}
IFrameSink::~IFrameSink() {}
IFrameSlicer::~IFrameSlicer() {}
IFrameSlices::~IFrameSlices() {}
IFrameSource::~IFrameSource() {}
IFrameSplitter::~IFrameSplitter() {}
IFunctionNodeBase::~IFunctionNodeBase() {}
IGroupSpectrum::~IGroupSpectrum() {}
IHydraNodeBase::~IHydraNodeBase() {}
IImpactResponse::~IImpactResponse() {}
IJoinNodeBase::~IJoinNodeBase() {}
IMeasure::~IMeasure() {}
INamed::~INamed() {}
INode::~INode() {}
IPlaneImpactResponse::~IPlaneImpactResponse() {}
IPointFieldSink::~IPointFieldSink() {}
IProcessor::~IProcessor() {}
IQueuedoutNodeBase::~IQueuedoutNodeBase() {}
IRecombinationModel::~IRecombinationModel() {}
IScalarFieldSink::~IScalarFieldSink() {}
ISemaphore::~ISemaphore() {}
ISinkNodeBase::~ISinkNodeBase() {}
ISlice::~ISlice() {}
ISliceFanout::~ISliceFanout() {}
ISliceFrame::~ISliceFrame() {}
ISliceFrameSink::~ISliceFrameSink() {}
ISliceStriper::~ISliceStriper() {}
ISourceNodeBase::~ISourceNodeBase() {}
ISplitNodeBase::~ISplitNodeBase() {}
IStripe::~IStripe() {}
IStripeSet::~IStripeSet() {}
ITensorForward::~ITensorForward() {}
ITensorPacker::~ITensorPacker() {}
ITensorSetFilter::~ITensorSetFilter() {}
ITensorSetUnpacker::~ITensorSetUnpacker() {}
ITerminal::~ITerminal() {}
ITiling::~ITiling() {}
ITrace::~ITrace() {}
ITraceRanker::~ITraceRanker() {}
IWaveform::~IWaveform() {}
IWaveformMap::~IWaveformMap() {}
IWireGenerator::~IWireGenerator() {}
IWireParameters::~IWireParameters() {}
IWireSchema::~IWireSchema() {}
IWireSource::~IWireSource() {}
IWireSummarizer::~IWireSummarizer() {}
IWireSummary::~IWireSummary() {}

