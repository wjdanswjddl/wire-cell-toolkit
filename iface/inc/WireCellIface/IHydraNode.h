/** A hydra has N input queues and M output queues.  

    A hydra node is intended to break or restablish synchronicity in a
    graph neighborhood.  Before implementing a hydra, think very
    carefully if it is needed and strongly prefer to use other
    categories.  In particular to create a synchronous N-to-M pattern
    consider to use a subgraph formed as a fanin-pipeline-fanout.

    Given that almost "anything goes" with hydra there are few
    requirements on the hydra node body function.

    - A hydra node body function shall not push to input queues nor
      pop from output queues.

    - Zero or more input queues may have zero or more elements.

    - Elements left on input queues after the body function call exits
      will persist to the next call.  

    - Any new input elements from the graph will be placed at the back
      of their respective queue.

    - Elements pushed to output queues shall not be expected to
      persist between calls.

    If the hydra node intends to preserve EOS-synchronization it must
    follow these requirements:

    - On receipt of an EOS from input queue the hydra node body
      function shall receive no further elements from the input queue
      until EOS is received on all input queues.

    - On sendinf of an EOS to an output queue the hydra node body
      function shall send EOS on all other output queues.

 */

#ifndef WIRECELL_IHYDRANODE
#define WIRECELL_IHYDRANODE

#include "WireCellIface/INode.h"
#include "WireCellUtil/TupleHelpers.h"

#include <boost/any.hpp>
#include <vector>
#include <deque>

namespace WireCell {

    /** Base hydra node class.
     *
     */
    class IHydraNodeBase : public INode {
       public:
        typedef std::shared_ptr<IHydraNodeBase> pointer;

        virtual ~IHydraNodeBase();

        typedef std::deque<boost::any> any_queue;
        typedef std::vector<any_queue> any_queue_vector;

        /// The calling signature:
        virtual bool operator()(any_queue_vector& anyinqs, any_queue_vector& anyoutqs) = 0;

        virtual NodeCategory category() { return hydraNode; }
    };

    /** The typed hydra interfaces are constructed as a 2x2 outer
        product: (in, out) X (T, V).
        
        Each input or output may be described as a tuple of queues (T)
        holding a variety of types (like join/split does for scalars)
        or as a vector of queues (V) holding identical types
        (like fanin/fanout does for scalars).
    */

    template <typename InputTuple>
    class IHydraInputT : virtual public IHydraNodeBase{
      public:

        // We use the tuple just to deliver the types of elements of each queue
        using input_tuple = InputTuple;
        // This is a type helper
        using input_shqed = shared_queued<input_tuple>;
        // The tuple<deque<shared_ptr<T>, deque<shared_ptr<T>, ...>
        // which is the input arg to the typed operator().
        using input_queues = typename input_shqed::shared_queued_tuple_type;

        // Return the names of the types this node takes as input.
        virtual std::vector<std::string> input_types()
        {
            tuple_helper<input_tuple> ih;
            return ih.type_names();
        }
    };

    template <typename OutputTuple>
    class IHydraOutputT : virtual public IHydraNodeBase{
      public:

        // We use the tuple just to deliver the types of elements of each queue
        using output_tuple = OutputTuple;
        // This is a type helper
        using output_shqed = shared_queued<output_tuple>;
        // The tuple<deque<shared_ptr<T>, deque<shared_ptr<T>, ...>
        // which is the output arg to the typed operator().
        using output_queues = typename output_shqed::shared_queued_tuple_type;

        // Return the names of the types this node produces as output.
        virtual std::vector<std::string> output_types()
        {
            tuple_helper<output_tuple> oh;
            return oh.type_names();
        }
    };

    template <typename InputType, int FaninMultiplicity = 1>
    class IHydraInputV : virtual public IHydraNodeBase {
      public:

        using input_type = InputType;
        using input_pointer = std::shared_ptr<const InputType>;
        using input_queue = std::deque<input_pointer>;
        // The type of the input argument to the typed operator()
        using input_queues = std::vector<input_queue>;

        // Return the names of the types this node takes as input.
        // Note: if subclass wants to supply FaninMultiplicity at
        // construction time, this needs to be overridden.
        virtual std::vector<std::string> input_types()
        {
            std::vector<std::string> ret(FaninMultiplicity, std::string(typeid(input_type).name()));
            return ret;
        }
    };

    template <typename OutputType, int FanoutMultiplicity = 1>
    class IHydraOutputV : virtual public IHydraNodeBase{
      public:

        using output_type = OutputType;
        using output_pointer = std::shared_ptr<const output_type>;
        using output_queue = std::deque<output_pointer>;
        // The type of the output argument to the typed operator()
        using output_queues = std::vector<output_queue>;

        // Return the names of the types this node produces as output.
        // Note: if subclass wants to supply FanoutMultiplicity at
        // construction time, this needs to be overridden.
        virtual std::vector<std::string> output_types() {
            std::vector<std::string> ret(FanoutMultiplicity, std::string(typeid(output_type).name()));
            return ret;
        }
    };
    

    /**
       The final typed layer forms the four elements of the
       (in,out) X (T,V) outer product.

       The cross meats in the operator().
     */

    // (inV, outV)
    template <typename InputType, typename OutputType, int FaninMultiplicity=1, int FanoutMultiplicity=1>
    class IHydraNodeVV : virtual public IHydraInputV<InputType, FaninMultiplicity>,
                         virtual public IHydraOutputV<OutputType, FanoutMultiplicity> {
      public:

        using HydraInput = IHydraInputV<InputType, FaninMultiplicity>;
        using HydraOutput = IHydraOutputV<OutputType, FanoutMultiplicity>;

        using typename IHydraNodeBase::any_queue_vector;
        using typename HydraInput::input_queue;
        using typename HydraInput::input_queues;
        using typename HydraOutput::output_queue;
        using typename HydraOutput::output_queues;

        // The "any" interface
        virtual bool operator()(any_queue_vector& anyinqs, any_queue_vector& anyoutqs)
        {
            // vector input
            const size_t isize = HydraInput::input_types().size();
            input_queues iqs(isize);
            for (size_t ind=0; ind<isize; ++ind) {
                iqs[ind] = boost::any_cast<input_queue>(anyinqs[ind]);
            }

            // cross over
            output_queues oqs;
            bool ok = (*this)(iqs, oqs);
            if (!ok) return false;
            
            // vector output
            const size_t osize = HydraOutput::output_types().size();
            anyoutqs.resize(osize);
            for (size_t ind = 0; ind < osize; ++ind) {
                anyoutqs[ind].insert(anyoutqs[ind].end(), oqs[ind].begin(), oqs[ind].end());
            }

            return true;
        }
            
        // typed interface
        virtual bool operator()(input_queues& iqs, output_queues& oqs) = 0;
    };


    // (inT, outT)
    template <typename InputTuple, typename OutputTuple>
    class IHydraNodeTT : virtual public IHydraInputT<InputTuple>,
                         virtual public IHydraOutputT<OutputTuple> {
      public:

        using HydraInput = IHydraInputT<InputTuple>;
        using HydraOutput = IHydraOutputT<OutputTuple>;

        using typename IHydraNodeBase::any_queue_vector;
        using typename HydraInput::input_queues;
        using typename HydraInput::input_shqed;
        using typename HydraOutput::output_queues;
        using typename HydraOutput::output_shqed;

        // The "any" interface
        virtual bool operator()(any_queue_vector& anyinqs, any_queue_vector& anyoutqs)
        {
            // tuple input
            input_shqed isq;
            auto iqs = isq.from_any_queue(anyinqs);

            // cross over
            output_queues oqs;
            bool ok = (*this)(iqs, oqs);
            if (!ok) return false;

            // tuple output
            output_shqed osq;
            anyoutqs = osq.as_any_queue(oqs);

            return true;
        }

        // typed interface
        virtual bool operator()(input_queues& iqs, output_queues& oqs) = 0;
    };

    // (inV, outT)
    template <typename InputType, typename OutputTuple, int FaninMultiplicity=1>
    class IHydraNodeVT : virtual public IHydraInputV<InputType, FaninMultiplicity>,
                         virtual public IHydraOutputT<OutputTuple> {
      public:

        using HydraInput = IHydraInputV<InputType, FaninMultiplicity>;
        using HydraOutput = IHydraOutputT<OutputTuple>;

        using typename IHydraNodeBase::any_queue_vector;
        using typename HydraInput::input_queue;
        using typename HydraInput::input_queues;
        using typename IHydraOutputT<OutputTuple>::output_queues;
        using typename IHydraOutputT<OutputTuple>::output_shqed;

        // The "any" interface
        virtual bool operator()(any_queue_vector& anyinqs, any_queue_vector& anyoutqs)
        {
            // vector input
            const size_t isize = HydraInput::input_types().size();
            input_queues iqs(isize);
            for (size_t ind=0; ind<isize; ++ind) {
                iqs[ind] = boost::any_cast<input_queue>(anyinqs[ind]);
            }

            // cross over
            output_queues oqs;
            bool ok = (*this)(iqs, oqs);
            if (!ok) return false;
            
            // tuple output
            output_shqed osq;
            anyoutqs = osq.as_any_queue(oqs);

            return true;
        }

        // typed interface
        virtual bool operator()(input_queues& iqs, output_queues& oqs) = 0;
    };

    // (inT, outV)
    template <typename InputTuple, typename OutputType, int FanoutMultiplicity=1>
    class IHydraNodeTV : virtual public IHydraInputT<InputTuple>,
                         virtual public IHydraOutputV<OutputType, FanoutMultiplicity> {
      public:

        using HydraInput = IHydraInputT<InputTuple>;
        using HydraOutput = IHydraOutputV<OutputType, FanoutMultiplicity>;

        using typename IHydraNodeBase::any_queue_vector;
        using typename HydraInput::input_queues;
        using typename HydraInput::input_shqed;
        using typename HydraOutput::output_queue;
        using typename HydraOutput::output_queues;

        // The "any" interface
        virtual bool operator()(any_queue_vector& anyinqs, any_queue_vector& anyoutqs)
        {
            // tuple input
            input_shqed isq;
            auto iqs = isq.from_any_queue(anyinqs);

            // cross over
            output_queues oqs;
            bool ok = (*this)(iqs, oqs);
            if (!ok) return false;
            
            // vector output
            const size_t osize = HydraOutput::output_types().size();
            anyoutqs.resize(osize);
            for (size_t ind = 0; ind < osize; ++ind) {
                anyoutqs[ind].insert(anyoutqs[ind].end(), oqs[ind].begin(), oqs[ind].end());
            }

            return true;
        }        

        // typed interface
        virtual bool operator()(input_queues& iqs, output_queues& oqs) = 0;
    };



}  // namespace WireCell

#endif
