#ifndef PIPELINE_INPUTS_H__
#define PIPELINE_INPUTS_H__

#include <boost/iterator/indirect_iterator.hpp>
#include <signals/Slots.h>
#include <util/foreach.h>
#include "Callbacks.h"
#include "Input.h"
#include "InputSignals.h"
#include "Logging.h"

namespace pipeline {

class MultiInput : public pipeline::InputBase {

public:

	/**
	 * Create a new multi-input.
	 */
	MultiInput();

	~MultiInput() {

		// delete the CallbacksBase pointers
		typedef std::pair<CallbacksBase*, ProcessNode*> cp_pair;
		foreach (cp_pair pair, _multiCallbacks)
			delete pair.first;
	}

	/**
	 * Register slots for backward signals with this multi-input. As more and
	 * more inputs are added to this multi-input, more and more slots will be
	 * created automatically.
	 *
	 * Example usage:
	 * <code>
	 * class Container : public ProcessNode {
	 *
	 *   Inputs<Data>        _inputs;
	 *   Slots<const Update> _update;
	 *
	 * public:
	 *
	 *   Container() {
	 *
	 *     registerInputs(_inputs);
	 *
	 *     // register the backward signal slots
	 *     _input.registerSlots(_update);
	 *   }
	 *
	 *   void sendUpdateSignals() {
	 *
	 *     // send an Update signal backwards for every input that was added
	 *     for (unsigned int i = 0; i < _inputs.size(); i++)
	 *       _update[i](Update());
	 *   }
	 * }
	 * </code>
	 *
	 * @param slot The signal slot to register.
	 */
	void registerSlots(signals::SlotsBase& slots);

	/**
	 * Register a ProcessNode method as a backward callback on a multi-input.
	 * This is a convenience wrapper that creates a ProcessNodeCallback object
	 * of the appropriate signal type for each input added to this multi-input.
	 * The callback will be called with the number of the input as second
	 * argument.
	 *
	 * Example usage:
	 * <code>
	 * class Container : public ProcessNode {
	 *
	 *   Inputs<Data> _inputs;
	 *
	 * public:
	 *
	 *   Container() {
	 *
	 *     registerInputs(_inputs);
	 *
	 *     _inputs.registerCallbacks(&Inputs::onModified, this);
	 *   }
	 *
	 * private:
	 *
	 *   void onModified(const Modified& signal, unsigned int numInput) {
	 *
	 *     std::cout << "the input " << numInput << " was modified!" << std::endl;
	 *   }
	 * }
	 * </code>
	 *
	 * @param callback    A function pointer to a callback method.
	 * @param processNode Pointer to the object, on which the callback method
	 *                    should be called.
	 */
	template <class T, typename SignalType>
	void registerCallbacks(void (T::*callback)(SignalType&, unsigned int), T* processNode, signals::CallbackInvocation invocation = signals::Exclusive) {

		boost::function<void(SignalType&, unsigned int)> multiCallback = boost::bind(callback, static_cast<T*>(processNode), _1, _2);

		_multiCallbacks.push_back(std::make_pair(new Callbacks<SignalType>(multiCallback, invocation), processNode));
	}

	/**
	 * Register an arbitrary callback as backward callback on this multi-input.
	 * Make sure that the callback will only be called as long as the given
	 * ProcessNode is still alive.
	 *
	 * @param callback A boost function object.
	 * @param processNode A ProcessNode to track.
	 */
	template <typename SignalType>
	void registerCallbacks(boost::function<void(SignalType&, unsigned int)> callback, ProcessNode* processNode, signals::CallbackInvocation invocation = signals::Exclusive) {

		_multiCallbacks.push_back(std::make_pair(new Callbacks<SignalType>(callback, invocation), processNode));
	}

	/**
	 * Register an arbitrary callback as backward callback on this multi-input.
	 *
	 * @param callback A boost function object.
	 */
	template <typename SignalType>
	void registerCallbacks(boost::function<void(SignalType&, unsigned int)> callback, signals::CallbackInvocation invocation = signals::Exclusive) {

		_multiCallbacks.push_back(std::make_pair(new Callbacks<SignalType>(callback, invocation), static_cast<ProcessNode*>(0)));
	}

	/**
	 * Try to add an output to this multi-input.
	 *
	 * @return true, if the output and multi-input are compatible and the output
	 *         has been added.
	 */
	virtual bool accept(OutputBase& output) = 0;

	/**
	 * Try to add a data pointer to this multi-input.
	 *
	 * @return true, if the pointer and multi-input are compatible and the
	 *         pointer has been added.
	 */
	virtual bool accept(boost::shared_ptr<Data> data) = 0;

	/**
	 * Remove all assigned outputs from this multi-input.
	 */
	virtual void clear() = 0;

	/**
	 * Remove all assigned outputs from this multi-input.
	 */
	void unset() { clear(); }

protected:

	/**
	 * Get a vector of all Slots that are registered with this multi-input.
	 * There is one Slots registered for each signal that should be sent
	 * backwards via this multi-input.
	 */
	std::vector<signals::SlotsBase*>& getSlots() {

		return _slots;
	}

	/**
	 * Get all the multi-callbacks with their optional process node.
	 */
	std::vector<std::pair<CallbacksBase*, ProcessNode*> >& getMultiCallbacks() {

		return _multiCallbacks;
	}

private:

	// a Slots pointer for each signal that can be sent backwards via this
	// multi-input (unsafe shared ownership, _slots have to survive this)
	std::vector<signals::SlotsBase*> _slots;

	// a CallbacksBase pointer for each registered multi-callback. Optionally, a
	// ProcessNode pointer can be given, which will be used for tracking, if
	// non-zero. (exclusive ownership)
	std::vector<std::pair<CallbacksBase*, ProcessNode*> > _multiCallbacks;
};

template <bool, typename DataType>
struct _input_added_dispatch {};

template <typename DataType>
struct _input_added_dispatch<true, DataType> {
	typedef InputAdded<DataType> value;
};

template <typename DataType>
struct _input_added_dispatch<false, DataType> {

	typedef InputAdded<Wrap<DataType> > value;
};

template <typename DataType>
class input_added_dispatch : public _input_added_dispatch<boost::is_base_of<Data, DataType>::value, DataType> {};

template <bool, typename DataType>
struct _input_added_to_shared_pointer_dispatch {};

template <typename DataType>
struct _input_added_to_shared_pointer_dispatch<true, DataType> {
	typedef InputAddedToSharedPointer<DataType> value;
};

template <typename DataType>
struct _input_added_to_shared_pointer_dispatch<false, DataType> {

	typedef InputAddedToSharedPointer<Wrap<DataType> > value;
};

template <typename DataType>
class input_added_to_shared_pointer_dispatch : public _input_added_to_shared_pointer_dispatch<boost::is_base_of<Data, DataType>::value, DataType> {};


template <typename DataType>
class Inputs : public MultiInput {

	typedef std::vector<boost::shared_ptr<Input<DataType> > > inputs_type;

	// this type is InputAdded<Wrap<DataType> > if DataType not base of Data,
	// InputAdded<DataType> otherwise
	typedef typename input_added_dispatch<DataType>::value                   input_added_type;
	typedef typename input_added_to_shared_pointer_dispatch<DataType>::value input_added_to_shared_pointer_type;

public:

	typedef boost::indirect_iterator<typename inputs_type::const_iterator, Input<DataType> > const_iterator;
	typedef boost::indirect_iterator<typename inputs_type::iterator,       Input<DataType> > iterator;

	Inputs() :
		_internalConnected(false) {

		_internalSender.registerSlot(_inputAdded);
		_internalSender.registerSlot(_inputAddedToSharedPointer);
		_internalSender.registerSlot(_inputsCleared);
	}

	bool accept(OutputBase& output) {

		return __accept(output);
	}

	bool accept(boost::shared_ptr<Data> data) {

		return __accept(data);
	}

	void clear() {

		// clear the slots
		foreach (signals::SlotsBase* slots, getSlots())
			slots->clear();

		// clear inputs
		_inputs.clear();

		// tell everyone
		_inputsCleared();
	}

	const Input<DataType>& operator[](unsigned int i) const {

		return *_inputs[i];
	}

	Input<DataType>& operator[](unsigned int i) {

		return *_inputs[i];
	}

	const const_iterator& begin() const {

		return const_iterator(_inputs.begin());
	}

	iterator begin() {

		return iterator(_inputs.begin());
	}

	const const_iterator& end() const {

		return const_iterator(_inputs.end());
	}

	iterator end() {

		return iterator(_inputs.end());
	}

	/**
	 * Get the current number of inputs.
	 */
	unsigned int size() const {

		return _inputs.size();
	}

	/**
	 * Return true if at least one input is present.
	 */
	operator bool() const {

		return size() > 0;
	}

	bool isSet() {

		return size() > 0;
	}

private:

	/**
	 * General accept routine, independet of type of assigned output (which can
	 * be OutputBase& or a shared_ptr<Data>.
	 */
	template <typename OutputType>
	bool __accept(OutputType& output) {

		LOG_ALL(pipelinelog) << "[" << typeName(this) << "] trying to accept output " << typeName(output) << std::endl;

		// create a new input
		boost::shared_ptr<Input<DataType> > newInput(new Input<DataType>());

		// store it, if it is compatible
		if (newInput->accept(output)) {

			LOG_ALL(pipelinelog) << "[" << typeName(this) << "] I can accept it" << std::endl;

			unsigned int numInput = _inputs.size();

			LOG_ALL(pipelinelog) << "[" << typeName(this) << "] registering slots:" << std::endl;

			foreach (signals::SlotsBase* slots, getSlots()) {

				unsigned int s = slots->addSlot();

				newInput->registerSlot((*slots)[s]);

				LOG_ALL(pipelinelog) << "[" << typeName(this) << "] " << typeName((*slots)[s]) << std::endl;
			}

			LOG_ALL(pipelinelog) << "[" << typeName(this) << "] registering callbacks:" << std::endl;

			typedef std::pair<CallbacksBase*, ProcessNode*> cp_pair;
			foreach (cp_pair pair, getMultiCallbacks()) {

				CallbacksBase* multiCallback = pair.first;
				ProcessNode*   processNode   = pair.second;

				if (processNode)
					multiCallback->registerAtInput(*newInput, numInput, processNode);
				else
					multiCallback->registerAtInput(*newInput, numInput);

				LOG_ALL(pipelinelog) << "[" << typeName(this) << "] " << typeName(multiCallback) << std::endl;
			}

			if (!_internalConnected) {

				// establish the internal signalling connections
				_internalSender.connect(getReceiver());

				_internalConnected = true;
			}

			LOG_ALL(pipelinelog) << "[" << typeName(this) << "] establishing signalling connections" << std::endl;

			establishingSignalling(output, *newInput);

			_inputs.push_back(newInput);

			return true;
		}

		return false;
	}

	void establishingSignalling(OutputBase& output, Input<DataType>& newInput) {

		// establish input-output signalling connections to Slots
		output.getSender().connect(newInput.getReceiver());
		newInput.getSender().connect(output.getReceiver());

		// establish input-output signalling connections to Slot
		output.getSender().connect(getReceiver());
		getSender().connect(output.getReceiver());

		LOG_ALL(pipelinelog) << "[" << typeName(this) << "] sending InputAdded" << std::endl;

		// inform about new input
		_inputAdded(input_added_type(newInput));
	}

	void establishingSignalling(boost::shared_ptr<Data>, Input<DataType>& newInput) {

		// no signalling for data pointers as inputs

		LOG_ALL(pipelinelog) << "[" << typeName(this) << "] sending InputAddedToSharedPointer" << std::endl;

		// inform about new input
		_inputAddedToSharedPointer(input_added_to_shared_pointer_type(newInput));
	}

	// inherited from InputBase, but not used in Inputs
	boost::shared_ptr<Data> getSharedDataPointer() const {

		return boost::shared_ptr<Data>();
	}

	// a list of the current inputs
	inputs_type _inputs;

	// slot to inform about input changes
	signals::Slot<const input_added_type>                   _inputAdded;
	signals::Slot<const input_added_to_shared_pointer_type> _inputAddedToSharedPointer;
	signals::Slot<const InputsCleared>                      _inputsCleared;

	// this sender is used to inform about changes in the input
	signals::Sender _internalSender;

	// indicate that the internal sender was connected already
	// TODO: It would be better if repeated connection to the same receiver does
	// not do anything.
	bool _internalConnected;
};

} // namespace pipeline

#endif // PIPELINE_INPUTS_H__

