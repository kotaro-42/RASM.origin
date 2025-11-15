/*******************************************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER $200k in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below $200k annual revenue or funding.

For entities with OVER $200k in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing@cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/10730637742483-RNBO-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
*******************************************************************************************************************/

#ifdef RNBO_LIB_PREFIX
#define STR_IMPL(A) #A
#define STR(A) STR_IMPL(A)
#define RNBO_LIB_INCLUDE(X) STR(RNBO_LIB_PREFIX/X)
#else
#define RNBO_LIB_INCLUDE(X) #X
#endif // RNBO_LIB_PREFIX
#ifdef RNBO_INJECTPLATFORM
#define RNBO_USECUSTOMPLATFORM
#include RNBO_INJECTPLATFORM
#endif // RNBO_INJECTPLATFORM

#include RNBO_LIB_INCLUDE(RNBO_Common.h)
#include RNBO_LIB_INCLUDE(RNBO_AudioSignal.h)

namespace RNBO {


#define trunc(x) ((Int)(x))
#define autoref auto&

#if defined(__GNUC__) || defined(__clang__)
    #define RNBO_RESTRICT __restrict__
#elif defined(_MSC_VER)
    #define RNBO_RESTRICT __restrict
#endif

#define FIXEDSIZEARRAYINIT(...) { }

template <class ENGINE = INTERNALENGINE> class rnbomatic : public PatcherInterfaceImpl {

friend class EngineCore;
friend class Engine;
friend class MinimalEngine<>;
public:

rnbomatic()
: _internalEngine(this)
{
}

~rnbomatic()
{
    deallocateSignals();
}

Index getNumMidiInputPorts() const {
    return 0;
}

void processMidiEvent(MillisecondTime , int , ConstByteArray , Index ) {}

Index getNumMidiOutputPorts() const {
    return 0;
}

void process(
    const SampleValue * const* inputs,
    Index numInputs,
    SampleValue * const* outputs,
    Index numOutputs,
    Index n
) {
    this->vs = n;
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr, true);
    SampleValue * out1 = (numOutputs >= 1 && outputs[0] ? outputs[0] : this->dummyBuffer);
    const SampleValue * in1 = (numInputs >= 1 && inputs[0] ? inputs[0] : this->zeroBuffer);
    this->pink_tilde_01_perform(this->signals[0], n);
    this->dspexpr_02_perform(this->signals[0], this->dspexpr_02_in2, this->signals[1], n);
    this->dspexpr_01_perform(this->signals[1], this->dspexpr_01_in2, out1, n);
    this->dspexpr_03_perform(in1, this->signals[1], n);

    this->average_tilde_01_perform(
        this->signals[1],
        this->average_tilde_01_windowSize,
        this->average_tilde_01_reset,
        this->signals[0],
        n
    );

    this->snapshot_01_perform(this->signals[0], n);
    this->stackprotect_perform(n);
    this->globaltransport_advance();
    this->advanceTime((ENGINE*)nullptr);
    this->audioProcessSampleCount += this->vs;
}

void prepareToProcess(number sampleRate, Index maxBlockSize, bool force) {
    RNBO_ASSERT(this->_isInitialized);

    if (this->maxvs < maxBlockSize || !this->didAllocateSignals) {
        Index i;

        for (i = 0; i < 2; i++) {
            this->signals[i] = resizeSignal(this->signals[i], this->maxvs, maxBlockSize);
        }

        this->globaltransport_tempo = resizeSignal(this->globaltransport_tempo, this->maxvs, maxBlockSize);
        this->globaltransport_state = resizeSignal(this->globaltransport_state, this->maxvs, maxBlockSize);
        this->zeroBuffer = resizeSignal(this->zeroBuffer, this->maxvs, maxBlockSize);
        this->dummyBuffer = resizeSignal(this->dummyBuffer, this->maxvs, maxBlockSize);
        this->didAllocateSignals = true;
    }

    const bool sampleRateChanged = sampleRate != this->sr;
    const bool maxvsChanged = maxBlockSize != this->maxvs;
    const bool forceDSPSetup = sampleRateChanged || maxvsChanged || force;

    if (sampleRateChanged || maxvsChanged) {
        this->vs = maxBlockSize;
        this->maxvs = maxBlockSize;
        this->sr = sampleRate;
        this->invsr = 1 / sampleRate;
    }

    this->average_tilde_01_dspsetup(forceDSPSetup);
    this->globaltransport_dspsetup(forceDSPSetup);

    if (sampleRateChanged)
        this->onSampleRateChanged(sampleRate);
}

number msToSamps(MillisecondTime ms, number sampleRate) {
    return ms * sampleRate * 0.001;
}

MillisecondTime sampsToMs(SampleIndex samps) {
    return samps * (this->invsr * 1000);
}

Index getNumInputChannels() const {
    return 1;
}

Index getNumOutputChannels() const {
    return 1;
}

DataRef* getDataRef(DataRefIndex index)  {
    switch (index) {
    case 0:
        {
        return addressOf(this->pink_tilde_01_rowsobj);
        break;
        }
    case 1:
        {
        return addressOf(this->average_tilde_01_bufferobj);
        break;
        }
    default:
        {
        return nullptr;
        }
    }
}

DataRefIndex getNumDataRefs() const {
    return 2;
}

void processDataViewUpdate(DataRefIndex index, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    if (index == 0) {
        this->pink_tilde_01_rows = reInitDataView(this->pink_tilde_01_rows, this->pink_tilde_01_rowsobj);
    }

    if (index == 1) {
        this->average_tilde_01_buffer = reInitDataView(this->average_tilde_01_buffer, this->average_tilde_01_bufferobj);
    }
}

void initialize() {
    RNBO_ASSERT(!this->_isInitialized);

    this->pink_tilde_01_rowsobj = initDataRef(
        this->pink_tilde_01_rowsobj,
        this->dataRefStrings->name0,
        true,
        this->dataRefStrings->file0,
        this->dataRefStrings->tag0
    );

    this->average_tilde_01_bufferobj = initDataRef(
        this->average_tilde_01_bufferobj,
        this->dataRefStrings->name1,
        true,
        this->dataRefStrings->file1,
        this->dataRefStrings->tag1
    );

    this->assign_defaults();
    this->applyState();
    this->pink_tilde_01_rowsobj->setIndex(0);
    this->pink_tilde_01_rows = new IntBuffer(this->pink_tilde_01_rowsobj);
    this->average_tilde_01_bufferobj->setIndex(1);
    this->average_tilde_01_buffer = new Float64Buffer(this->average_tilde_01_bufferobj);
    this->initializeObjects();
    this->allocateDataRefs();
    this->startup();
    this->_isInitialized = true;
}

void getPreset(PatcherStateInterface& preset) {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);
    preset["__presetid"] = "rnbo";
    this->param_01_getPresetValue(getSubState(preset, "volume"));
    this->param_02_getPresetValue(getSubState(preset, "sensitivity"));
    this->param_03_getPresetValue(getSubState(preset, "responsiveness"));
    this->param_04_getPresetValue(getSubState(preset, "dynamics"));
    this->param_05_getPresetValue(getSubState(preset, "release"));
}

void setPreset(MillisecondTime time, PatcherStateInterface& preset) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->param_01_setPresetValue(getSubState(preset, "volume"));
    this->param_02_setPresetValue(getSubState(preset, "sensitivity"));
    this->param_03_setPresetValue(getSubState(preset, "responsiveness"));
    this->param_04_setPresetValue(getSubState(preset, "dynamics"));
    this->param_05_setPresetValue(getSubState(preset, "release"));
}

void setParameterValue(ParameterIndex index, ParameterValue v, MillisecondTime time) {
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case 0:
        {
        this->param_01_value_set(v);
        break;
        }
    case 1:
        {
        this->param_02_value_set(v);
        break;
        }
    case 2:
        {
        this->param_03_value_set(v);
        break;
        }
    case 3:
        {
        this->param_04_value_set(v);
        break;
        }
    case 4:
        {
        this->param_05_value_set(v);
        break;
        }
    }
}

void processParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValue(index, value, time);
}

void processParameterBangEvent(ParameterIndex index, MillisecondTime time) {
    this->setParameterValue(index, this->getParameterValue(index), time);
}

void processNormalizedParameterEvent(ParameterIndex index, ParameterValue value, MillisecondTime time) {
    this->setParameterValueNormalized(index, value, time);
}

ParameterValue getParameterValue(ParameterIndex index)  {
    switch (index) {
    case 0:
        {
        return this->param_01_value;
        }
    case 1:
        {
        return this->param_02_value;
        }
    case 2:
        {
        return this->param_03_value;
        }
    case 3:
        {
        return this->param_04_value;
        }
    case 4:
        {
        return this->param_05_value;
        }
    default:
        {
        return 0;
        }
    }
}

ParameterIndex getNumSignalInParameters() const {
    return 0;
}

ParameterIndex getNumSignalOutParameters() const {
    return 0;
}

ParameterIndex getNumParameters() const {
    return 5;
}

ConstCharPointer getParameterName(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "volume";
        }
    case 1:
        {
        return "sensitivity";
        }
    case 2:
        {
        return "responsiveness";
        }
    case 3:
        {
        return "dynamics";
        }
    case 4:
        {
        return "release";
        }
    default:
        {
        return "bogus";
        }
    }
}

ConstCharPointer getParameterId(ParameterIndex index) const {
    switch (index) {
    case 0:
        {
        return "volume";
        }
    case 1:
        {
        return "sensitivity";
        }
    case 2:
        {
        return "responsiveness";
        }
    case 3:
        {
        return "dynamics";
        }
    case 4:
        {
        return "release";
        }
    default:
        {
        return "bogus";
        }
    }
}

void getParameterInfo(ParameterIndex index, ParameterInfo * info) const {
    {
        switch (index) {
        case 0:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 0;
            info->min = 0;
            info->max = 1;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 1:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 40;
            info->min = 20;
            info->max = 80;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 2:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 5;
            info->min = 0;
            info->max = 10;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 3:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 2;
            info->min = 0;
            info->max = 4;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        case 4:
            {
            info->type = ParameterTypeNumber;
            info->initialValue = 5;
            info->min = 0;
            info->max = 10;
            info->exponent = 1;
            info->steps = 0;
            info->debug = false;
            info->saveable = true;
            info->transmittable = true;
            info->initialized = true;
            info->visible = true;
            info->displayName = "";
            info->unit = "";
            info->ioType = IOTypeUndefined;
            info->signalIndex = INVALID_INDEX;
            break;
            }
        }
    }
}

ParameterValue applyStepsToNormalizedParameterValue(ParameterValue normalizedValue, int steps) const {
    if (steps == 1) {
        if (normalizedValue > 0) {
            normalizedValue = 1.;
        }
    } else {
        ParameterValue oneStep = (number)1. / (steps - 1);
        ParameterValue numberOfSteps = rnbo_fround(normalizedValue / oneStep * 1 / (number)1) * (number)1;
        normalizedValue = numberOfSteps * oneStep;
    }

    return normalizedValue;
}

ParameterValue convertToNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    case 0:
        {
        {
            value = (value < 0 ? 0 : (value > 1 ? 1 : value));
            ParameterValue normalizedValue = (value - 0) / (1 - 0);
            return normalizedValue;
        }
        }
    case 3:
        {
        {
            value = (value < 0 ? 0 : (value > 4 ? 4 : value));
            ParameterValue normalizedValue = (value - 0) / (4 - 0);
            return normalizedValue;
        }
        }
    case 2:
    case 4:
        {
        {
            value = (value < 0 ? 0 : (value > 10 ? 10 : value));
            ParameterValue normalizedValue = (value - 0) / (10 - 0);
            return normalizedValue;
        }
        }
    case 1:
        {
        {
            value = (value < 20 ? 20 : (value > 80 ? 80 : value));
            ParameterValue normalizedValue = (value - 20) / (80 - 20);
            return normalizedValue;
        }
        }
    default:
        {
        return value;
        }
    }
}

ParameterValue convertFromNormalizedParameterValue(ParameterIndex index, ParameterValue value) const {
    value = (value < 0 ? 0 : (value > 1 ? 1 : value));

    switch (index) {
    case 0:
        {
        {
            {
                return 0 + value * (1 - 0);
            }
        }
        }
    case 3:
        {
        {
            {
                return 0 + value * (4 - 0);
            }
        }
        }
    case 2:
    case 4:
        {
        {
            {
                return 0 + value * (10 - 0);
            }
        }
        }
    case 1:
        {
        {
            {
                return 20 + value * (80 - 20);
            }
        }
        }
    default:
        {
        return value;
        }
    }
}

ParameterValue constrainParameterValue(ParameterIndex index, ParameterValue value) const {
    switch (index) {
    case 0:
        {
        return this->param_01_value_constrain(value);
        }
    case 1:
        {
        return this->param_02_value_constrain(value);
        }
    case 2:
        {
        return this->param_03_value_constrain(value);
        }
    case 3:
        {
        return this->param_04_value_constrain(value);
        }
    case 4:
        {
        return this->param_05_value_constrain(value);
        }
    default:
        {
        return value;
        }
    }
}

void processNumMessage(MessageTag , MessageTag , MillisecondTime , number ) {}

void processListMessage(MessageTag , MessageTag , MillisecondTime , const list& ) {}

void processBangMessage(MessageTag , MessageTag , MillisecondTime ) {}

MessageTagInfo resolveTag(MessageTag tag) const {
    switch (tag) {

    }

    return "";
}

MessageIndex getNumMessages() const {
    return 0;
}

const MessageInfo& getMessageInfo(MessageIndex index) const {
    switch (index) {

    }

    return NullMessageInfo;
}

protected:

		
void advanceTime(EXTERNALENGINE*) {}
void advanceTime(INTERNALENGINE*) {
	_internalEngine.advanceTime(sampstoms(this->vs));
}

void processInternalEvents(MillisecondTime time) {
	_internalEngine.processEventsUntil(time);
}

void updateTime(MillisecondTime time, INTERNALENGINE*, bool inProcess = false) {
	if (time == TimeNow) time = getPatcherTime();
	processInternalEvents(inProcess ? time + sampsToMs(this->vs) : time);
	updateTime(time, (EXTERNALENGINE*)nullptr);
}

rnbomatic* operator->() {
    return this;
}
const rnbomatic* operator->() const {
    return this;
}
rnbomatic* getTopLevelPatcher() {
    return this;
}

void cancelClockEvents()
{
    getEngine()->flushClockEvents(this, 1646922831, false);
}

template<typename LISTTYPE = list> void listquicksort(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    if (l < h) {
        Int p = (Int)(this->listpartition(arr, sortindices, l, h, ascending));
        this->listquicksort(arr, sortindices, l, p - 1, ascending);
        this->listquicksort(arr, sortindices, p + 1, h, ascending);
    }
}

template<typename LISTTYPE = list> Int listpartition(LISTTYPE& arr, LISTTYPE& sortindices, Int l, Int h, bool ascending) {
    number x = arr[(Index)h];
    Int i = (Int)(l - 1);

    for (Int j = (Int)(l); j <= h - 1; j++) {
        bool asc = (bool)((bool)(ascending) && arr[(Index)j] <= x);
        bool desc = (bool)((bool)(!(bool)(ascending)) && arr[(Index)j] >= x);

        if ((bool)(asc) || (bool)(desc)) {
            i++;
            this->listswapelements(arr, i, j);
            this->listswapelements(sortindices, i, j);
        }
    }

    i++;
    this->listswapelements(arr, i, h);
    this->listswapelements(sortindices, i, h);
    return i;
}

template<typename LISTTYPE = list> void listswapelements(LISTTYPE& arr, Int a, Int b) {
    auto tmp = arr[(Index)a];
    arr[(Index)a] = arr[(Index)b];
    arr[(Index)b] = tmp;
}

inline number safediv(number num, number denom) {
    return (denom == 0.0 ? 0.0 : num / denom);
}

number maximum(number x, number y) {
    return (x < y ? y : x);
}

number safepow(number base, number exponent) {
    return fixnan(rnbo_pow(base, exponent));
}

number scale(
    number x,
    number lowin,
    number hiin,
    number lowout,
    number highout,
    number pow
) {
    auto inscale = this->safediv(1., hiin - lowin);
    number outdiff = highout - lowout;
    number value = (x - lowin) * inscale;

    if (pow != 1) {
        if (value > 0)
            value = this->safepow(value, pow);
        else
            value = -this->safepow(-value, pow);
    }

    value = value * outdiff + lowout;
    return value;
}

number mstosamps(MillisecondTime ms) {
    return ms * this->sr * 0.001;
}

MillisecondTime sampstoms(number samps) {
    return samps * 1000 / this->sr;
}

void param_01_value_set(number v) {
    v = this->param_01_value_constrain(v);
    this->param_01_value = v;
    this->sendParameter(0, false);

    if (this->param_01_value != this->param_01_lastValue) {
        this->getEngine()->presetTouched();
        this->param_01_lastValue = this->param_01_value;
    }

    this->dspexpr_01_in2_set(v);
}

void param_02_value_set(number v) {
    v = this->param_02_value_constrain(v);
    this->param_02_value = v;
    this->sendParameter(1, false);

    if (this->param_02_value != this->param_02_lastValue) {
        this->getEngine()->presetTouched();
        this->param_02_lastValue = this->param_02_value;
    }

    this->expr_03_in1_set(v);
    this->scale_01_inlow_set(v);
}

void param_03_value_set(number v) {
    v = this->param_03_value_constrain(v);
    this->param_03_value = v;
    this->sendParameter(2, false);

    if (this->param_03_value != this->param_03_lastValue) {
        this->getEngine()->presetTouched();
        this->param_03_lastValue = this->param_03_value;
    }

    this->expr_04_in1_set(v);
}

void param_04_value_set(number v) {
    v = this->param_04_value_constrain(v);
    this->param_04_value = v;
    this->sendParameter(3, false);

    if (this->param_04_value != this->param_04_lastValue) {
        this->getEngine()->presetTouched();
        this->param_04_lastValue = this->param_04_value;
    }

    this->expr_05_in1_set(v);
}

void param_05_value_set(number v) {
    v = this->param_05_value_constrain(v);
    this->param_05_value = v;
    this->sendParameter(4, false);

    if (this->param_05_value != this->param_05_lastValue) {
        this->getEngine()->presetTouched();
        this->param_05_lastValue = this->param_05_value;
    }

    this->expr_06_in1_set(v);
}

MillisecondTime getPatcherTime() const {
    return this->_currentTime;
}

void snapshot_01_out_set(number v) {
    this->snapshot_01_out = v;
    this->expr_02_in1_set(v);
}

void deallocateSignals() {
    Index i;

    for (i = 0; i < 2; i++) {
        this->signals[i] = freeSignal(this->signals[i]);
    }

    this->globaltransport_tempo = freeSignal(this->globaltransport_tempo);
    this->globaltransport_state = freeSignal(this->globaltransport_state);
    this->zeroBuffer = freeSignal(this->zeroBuffer);
    this->dummyBuffer = freeSignal(this->dummyBuffer);
}

Index getMaxBlockSize() const {
    return this->maxvs;
}

number getSampleRate() const {
    return this->sr;
}

bool hasFixedVectorSize() const {
    return false;
}

void setProbingTarget(MessageTag ) {}

void fillDataRef(DataRefIndex , DataRef& ) {}

void zeroDataRef(DataRef& ref) {
    ref->setZero();
}

void allocateDataRefs() {
    this->pink_tilde_01_rows->requestSize(16);
    this->pink_tilde_01_rows = this->pink_tilde_01_rows->allocateIfNeeded();

    if (this->pink_tilde_01_rowsobj->hasRequestedSize()) {
        if (this->pink_tilde_01_rowsobj->wantsFill())
            this->zeroDataRef(this->pink_tilde_01_rowsobj);

        this->getEngine()->sendDataRefUpdated(0);
    }

    this->average_tilde_01_buffer = this->average_tilde_01_buffer->allocateIfNeeded();

    if (this->average_tilde_01_bufferobj->hasRequestedSize()) {
        if (this->average_tilde_01_bufferobj->wantsFill())
            this->zeroDataRef(this->average_tilde_01_bufferobj);

        this->getEngine()->sendDataRefUpdated(1);
    }
}

void initializeObjects() {
    this->pink_tilde_01_init();
    this->average_tilde_01_init();
}

Index getIsMuted()  {
    return this->isMuted;
}

void setIsMuted(Index v)  {
    this->isMuted = v;
}

void onSampleRateChanged(double ) {}

void extractState(PatcherStateInterface& ) {}

void applyState() {}

void processClockEvent(MillisecondTime time, ClockId index, bool hasValue, ParameterValue value) {
    RNBO_UNUSED(hasValue);
    this->updateTime(time, (ENGINE*)nullptr);

    switch (index) {
    case 1646922831:
        {
        this->snapshot_01_out_set(value);
        break;
        }
    }
}

void processOutletAtCurrentTime(EngineLink* , OutletIndex , ParameterValue ) {}

void processOutletEvent(
    EngineLink* sender,
    OutletIndex index,
    ParameterValue value,
    MillisecondTime time
) {
    this->updateTime(time, (ENGINE*)nullptr);
    this->processOutletAtCurrentTime(sender, index, value);
}

void sendOutlet(OutletIndex index, ParameterValue value) {
    this->getEngine()->sendOutlet(this, index, value);
}

void startup() {
    this->updateTime(this->getEngine()->getCurrentTime(), (ENGINE*)nullptr);

    {
        this->scheduleParamInit(0, 0);
    }

    {
        this->scheduleParamInit(1, 0);
    }

    {
        this->scheduleParamInit(2, 0);
    }

    {
        this->scheduleParamInit(3, 0);
    }

    {
        this->scheduleParamInit(4, 0);
    }

    this->processParamInitEvents();
}

number param_01_value_constrain(number v) const {
    v = (v > 1 ? 1 : (v < 0 ? 0 : v));
    return v;
}

void dspexpr_01_in2_set(number v) {
    this->dspexpr_01_in2 = v;
}

number param_02_value_constrain(number v) const {
    v = (v > 80 ? 80 : (v < 20 ? 20 : v));
    return v;
}

void scale_01_inhigh_set(number v) {
    this->scale_01_inhigh = v;
}

void expr_03_out1_set(number v) {
    this->expr_03_out1 = v;
    this->scale_01_inhigh_set(this->expr_03_out1);
}

void expr_03_in1_set(number in1) {
    this->expr_03_in1 = in1;
    this->expr_03_out1_set(this->expr_03_in1 + this->expr_03_in2);//#map:expr_obj-17:1
}

void scale_01_inlow_set(number v) {
    this->scale_01_inlow = v;
}

number param_03_value_constrain(number v) const {
    v = (v > 10 ? 10 : (v < 0 ? 0 : v));
    return v;
}

void slide_01_up_set(number v) {
    this->slide_01_up = v;
}

void expr_04_out1_set(number v) {
    this->expr_04_out1 = v;
    this->slide_01_up_set(this->expr_04_out1);
}

void expr_04_in1_set(number in1) {
    this->expr_04_in1 = in1;
    this->expr_04_out1_set(2 * this->expr_04_in1 * this->expr_04_in1 + 10);//#map:expr_obj-29:1
}

number param_04_value_constrain(number v) const {
    v = (v > 4 ? 4 : (v < 0 ? 0 : v));
    return v;
}

void expr_03_in2_set(number v) {
    this->expr_03_in2 = v;
}

void expr_05_out1_set(number v) {
    this->expr_05_out1 = v;
    this->expr_03_in2_set(this->expr_05_out1);
}

void expr_05_in1_set(number in1) {
    this->expr_05_in1 = in1;
    this->expr_05_out1_set((this->expr_05_in1 + 4) * 5);//#map:expr_obj-15:1
}

number param_05_value_constrain(number v) const {
    v = (v > 10 ? 10 : (v < 0 ? 0 : v));
    return v;
}

void slide_01_down_set(number v) {
    this->slide_01_down = v;
}

void expr_06_out1_set(number v) {
    this->expr_06_out1 = v;
    this->slide_01_down_set(this->expr_06_out1);
}

void expr_06_in1_set(number in1) {
    this->expr_06_in1 = in1;
    this->expr_06_out1_set(2 * this->expr_06_in1 * this->expr_06_in1 + 10);//#map:expr_obj-30:1
}

void dspexpr_02_in2_set(number v) {
    this->dspexpr_02_in2 = v;
}

void slide_01_out1_set(number v) {
    this->dspexpr_02_in2_set(v);
}

void slide_01_x_set(number x) {
    this->slide_01_x = x;
    auto down = this->slide_01_down;
    auto up = this->slide_01_up;
    number temp = x - this->slide_01_prev;
    auto iup = this->safediv(1., this->maximum(1., rnbo_abs(up)));
    auto idown = this->safediv(1., this->maximum(1., rnbo_abs(down)));
    this->slide_01_prev = this->slide_01_prev + ((x > this->slide_01_prev ? iup : idown)) * temp;

    {
        this->slide_01_out1_set(this->slide_01_prev);
        return;
    }
}

void expr_01_out1_set(number v) {
    this->expr_01_out1 = v;
    this->slide_01_x_set(this->expr_01_out1);
}

void expr_01_in1_set(number in1) {
    this->expr_01_in1 = in1;

    this->expr_01_out1_set(
        (this->expr_01_in1 > this->expr_01_in3 ? this->expr_01_in3 : (this->expr_01_in1 < this->expr_01_in2 ? this->expr_01_in2 : this->expr_01_in1))
    );//#map:clip_obj-22:1
}

void expr_01_in2_set(number v) {
    this->expr_01_in2 = v;
}

void expr_01_in3_set(number v) {
    this->expr_01_in3 = v;
}

template<typename LISTTYPE> void scale_01_out_set(const LISTTYPE& v) {
    this->scale_01_out = jsCreateListCopy(v);

    {
        if (v->length > 2)
            this->expr_01_in3_set(v[2]);

        if (v->length > 1)
            this->expr_01_in2_set(v[1]);

        number converted = (v->length > 0 ? v[0] : 0);
        this->expr_01_in1_set(converted);
    }
}

template<typename LISTTYPE> void scale_01_input_set(const LISTTYPE& v) {
    this->scale_01_input = jsCreateListCopy(v);
    list tmp = {};

    for (Index i = 0; i < v->length; i++) {
        tmp->push(this->scale(
            v[(Index)i],
            this->scale_01_inlow,
            this->scale_01_inhigh,
            this->scale_01_outlow,
            this->scale_01_outhigh,
            this->scale_01_power
        ));
    }

    this->scale_01_out_set(tmp);
}

void expr_02_out1_set(number v) {
    this->expr_02_out1 = v;

    {
        listbase<number, 1> converted = {this->expr_02_out1};
        this->scale_01_input_set(converted);
    }
}

void expr_02_in1_set(number in1) {
    this->expr_02_in1 = in1;

    this->expr_02_out1_set(20 * rnbo_log10(
        (this->maximum(this->expr_02_in1, 0.0001) <= 0.0000000001 ? 0.0000000001 : this->maximum(this->expr_02_in1, 0.0001))
    ) + 100);//#map:expr_obj-8:1
}

void pink_tilde_01_perform(SampleValue * out1, Index n) {
    auto __pink_tilde_01_numRows = this->pink_tilde_01_numRows;
    auto __pink_tilde_01_runningSum = this->pink_tilde_01_runningSum;
    auto __pink_tilde_01_indexMask = this->pink_tilde_01_indexMask;
    auto __pink_tilde_01_index = this->pink_tilde_01_index;
    number tmp = 0x007fffff;
    Index i;

    for (i = 0; i < (Index)n; i++) {
        Int newRandom;
        Int sum;
        __pink_tilde_01_index = (BinOpInt)((BinOpInt)(__pink_tilde_01_index + 1) & __pink_tilde_01_indexMask);

        if (__pink_tilde_01_index != 0) {
            Int numZeros = 0;
            Int n = (Int)(__pink_tilde_01_index);

            while (((BinOpInt)((BinOpInt)n & (BinOpInt)1)) == 0) {
                n = (BinOpInt)((BinOpInt)n >> imod_nocast((UBinOpInt)1, 32));
                numZeros++;
            }

            __pink_tilde_01_runningSum -= this->pink_tilde_01_rows[(Index)numZeros];
            newRandom = (BinOpInt)((BinOpInt)this->pink_tilde_01_generateRandomNumber() & (BinOpInt)0x007fffff);
            __pink_tilde_01_runningSum += newRandom;
            this->pink_tilde_01_rows[(Index)numZeros] = newRandom;
        }

        newRandom = (BinOpInt)((BinOpInt)this->pink_tilde_01_generateRandomNumber() & (BinOpInt)0x007fffff);
        sum = __pink_tilde_01_runningSum + newRandom;
        out1[(Index)i] = sum / (tmp * __pink_tilde_01_numRows) * 2. - 1.;
    }

    this->pink_tilde_01_index = __pink_tilde_01_index;
    this->pink_tilde_01_runningSum = __pink_tilde_01_runningSum;
}

void dspexpr_02_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
    }
}

void dspexpr_01_perform(const Sample * in1, number in2, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = in1[(Index)i] * in2;//#map:_###_obj_###_:1
    }
}

void dspexpr_03_perform(const Sample * in1, SampleValue * out1, Index n) {
    Index i;

    for (i = 0; i < (Index)n; i++) {
        out1[(Index)i] = rnbo_abs(in1[(Index)i]);//#map:_###_obj_###_:1
    }
}

void average_tilde_01_perform(
    const Sample * x,
    number windowSize,
    number reset,
    SampleValue * out1,
    Index n
) {
    RNBO_UNUSED(reset);
    RNBO_UNUSED(windowSize);
    auto __average_tilde_01_resetFlag = this->average_tilde_01_resetFlag;
    Index i;

    for (i = 0; i < (Index)n; i++) {
        this->average_tilde_01_setwindowsize(1000);
        __average_tilde_01_resetFlag = 0;

        if (this->average_tilde_01_wantsReset == 1) {
            this->average_tilde_01_doReset();
        }

        this->average_tilde_01_accum += x[(Index)i];
        this->average_tilde_01_buffer[(Index)this->average_tilde_01_bufferPos] = x[(Index)i];
        number bufferSize = this->average_tilde_01_buffer->getSize();

        if (this->average_tilde_01_effectiveWindowSize < this->average_tilde_01_currentWindowSize) {
            this->average_tilde_01_effectiveWindowSize++;
        } else {
            number bufferReadPos = this->average_tilde_01_bufferPos - this->average_tilde_01_effectiveWindowSize;

            while (bufferReadPos < 0)
                bufferReadPos += bufferSize;

            this->average_tilde_01_accum -= this->average_tilde_01_buffer[(Index)bufferReadPos];
        }

        this->average_tilde_01_bufferPos++;

        if (this->average_tilde_01_bufferPos >= bufferSize) {
            this->average_tilde_01_bufferPos -= bufferSize;
        }

        out1[(Index)i] = this->average_tilde_01_accum / this->average_tilde_01_effectiveWindowSize;
    }

    this->average_tilde_01_resetFlag = __average_tilde_01_resetFlag;
}

void snapshot_01_perform(const SampleValue * input_signal, Index n) {
    auto __snapshot_01_lastValue = this->snapshot_01_lastValue;
    auto __snapshot_01_calc = this->snapshot_01_calc;
    auto __snapshot_01_count = this->snapshot_01_count;
    auto __snapshot_01_nextTime = this->snapshot_01_nextTime;
    auto __snapshot_01_interval = this->snapshot_01_interval;
    number timeInSamples = this->msToSamps(__snapshot_01_interval, this->sr);

    if (__snapshot_01_interval > 0) {
        for (Index i = 0; i < n; i++) {
            if (__snapshot_01_nextTime <= __snapshot_01_count + (SampleIndex)(i)) {
                {
                    __snapshot_01_calc = input_signal[(Index)i];
                }

                this->getEngine()->scheduleClockEventWithValue(
                    this,
                    1646922831,
                    this->sampsToMs((SampleIndex)(this->vs)) + this->_currentTime,
                    __snapshot_01_calc
                );;

                __snapshot_01_calc = 0;
                __snapshot_01_nextTime += timeInSamples;
            }
        }

        __snapshot_01_count += this->vs;
    }

    __snapshot_01_lastValue = input_signal[(Index)(n - 1)];
    this->snapshot_01_nextTime = __snapshot_01_nextTime;
    this->snapshot_01_count = __snapshot_01_count;
    this->snapshot_01_calc = __snapshot_01_calc;
    this->snapshot_01_lastValue = __snapshot_01_lastValue;
}

void stackprotect_perform(Index n) {
    RNBO_UNUSED(n);
    auto __stackprotect_count = this->stackprotect_count;
    __stackprotect_count = 0;
    this->stackprotect_count = __stackprotect_count;
}

void pink_tilde_01_init() {
    this->pink_tilde_01_last = systemticks();
    this->pink_tilde_01_index = 0;
    this->pink_tilde_01_indexMask = ((BinOpInt)((BinOpInt)1 << imod_nocast((UBinOpInt)this->pink_tilde_01_numRows, 32))) - 1;
    number tmp = this->pink_tilde_01_randomShift - 1;
    this->pink_tilde_01_scalar = (number)1 / ((this->pink_tilde_01_numRows + 1) * ((BinOpInt)((BinOpInt)1 << imod_nocast((UBinOpInt)tmp, 32))));
    this->pink_tilde_01_runningSum = 0;
}

Index pink_tilde_01_generateRandomNumber() {
    this->pink_tilde_01_last = (BinOpInt)((BinOpInt)(rnbo_imul(this->pink_tilde_01_last, 196314165) + 907633515) | (BinOpInt)0);
    Int tmp = (Int)(this->pink_tilde_01_last);
    return tmp;
}

void average_tilde_01_init() {
    this->average_tilde_01_currentWindowSize = this->sr;
    this->average_tilde_01_buffer->requestSize(this->sr + 1, 1);
    this->average_tilde_01_doReset();
}

void average_tilde_01_doReset() {
    this->average_tilde_01_accum = 0;
    this->average_tilde_01_effectiveWindowSize = 0;
    this->average_tilde_01_bufferPos = 0;
    this->average_tilde_01_wantsReset = 0;
}

void average_tilde_01_setwindowsize(number wsize) {
    wsize = trunc(wsize);

    if (wsize != this->average_tilde_01_currentWindowSize && wsize > 0 && wsize <= this->sr) {
        this->average_tilde_01_currentWindowSize = wsize;
        this->average_tilde_01_wantsReset = 1;
    }
}

void average_tilde_01_dspsetup(bool force) {
    if ((bool)(this->average_tilde_01_setupDone) && (bool)(!(bool)(force)))
        return;

    this->average_tilde_01_wantsReset = 1;

    if (this->sr > this->average_tilde_01_buffer->getSize()) {
        this->average_tilde_01_buffer->setSize(this->sr + 1);
        updateDataRef(this, this->average_tilde_01_buffer);
    }

    this->average_tilde_01_setupDone = true;
}

void param_01_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_01_value;
}

void param_01_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_01_value_set(preset["value"]);
}

void param_02_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_02_value;
}

void param_02_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_02_value_set(preset["value"]);
}

void param_03_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_03_value;
}

void param_03_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_03_value_set(preset["value"]);
}

void param_04_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_04_value;
}

void param_04_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_04_value_set(preset["value"]);
}

void param_05_getPresetValue(PatcherStateInterface& preset) {
    preset["value"] = this->param_05_value;
}

void param_05_setPresetValue(PatcherStateInterface& preset) {
    if ((bool)(stateIsEmpty(preset)))
        return;

    this->param_05_value_set(preset["value"]);
}

void globaltransport_advance() {}

void globaltransport_dspsetup(bool ) {}

bool stackprotect_check() {
    this->stackprotect_count++;

    if (this->stackprotect_count > 128) {
        console->log("STACK OVERFLOW DETECTED - stopped processing branch !");
        return true;
    }

    return false;
}

Index getPatcherSerial() const {
    return 0;
}

void sendParameter(ParameterIndex index, bool ignoreValue) {
    this->getEngine()->notifyParameterValueChanged(index, (ignoreValue ? 0 : this->getParameterValue(index)), ignoreValue);
}

void scheduleParamInit(ParameterIndex index, Index order) {
    this->paramInitIndices->push(index);
    this->paramInitOrder->push(order);
}

void processParamInitEvents() {
    this->listquicksort(
        this->paramInitOrder,
        this->paramInitIndices,
        0,
        (int)(this->paramInitOrder->length - 1),
        true
    );

    for (Index i = 0; i < this->paramInitOrder->length; i++) {
        this->getEngine()->scheduleParameterBang(this->paramInitIndices[i], 0);
    }
}

void updateTime(MillisecondTime time, EXTERNALENGINE* engine, bool inProcess = false) {
    RNBO_UNUSED(inProcess);
    RNBO_UNUSED(engine);
    this->_currentTime = time;
    auto offset = rnbo_fround(this->msToSamps(time - this->getEngine()->getCurrentTime(), this->sr));

    if (offset >= (SampleIndex)(this->vs))
        offset = (SampleIndex)(this->vs) - 1;

    if (offset < 0)
        offset = 0;

    this->sampleOffsetIntoNextAudioBuffer = (Index)(offset);
}

void assign_defaults()
{
    dspexpr_01_in1 = 0;
    dspexpr_01_in2 = 1;
    dspexpr_02_in1 = 0;
    dspexpr_02_in2 = 0;
    slide_01_x = 0;
    slide_01_up = 0;
    slide_01_down = 0;
    expr_01_in1 = 0;
    expr_01_in2 = 0;
    expr_01_in3 = 1;
    expr_01_out1 = 0;
    scale_01_inlow = 40;
    scale_01_inhigh = 70;
    scale_01_outlow = 0;
    scale_01_outhigh = 1;
    scale_01_power = 1;
    expr_02_in1 = 0;
    expr_02_out1 = 0;
    snapshot_01_interval = 50;
    snapshot_01_out = 0;
    average_tilde_01_x = 0;
    average_tilde_01_windowSize = 1000;
    average_tilde_01_reset = 0;
    dspexpr_03_in1 = 0;
    param_01_value = 0;
    expr_03_in1 = 0;
    expr_03_in2 = 0;
    expr_03_out1 = 0;
    param_02_value = 40;
    expr_04_in1 = 0;
    expr_04_out1 = 0;
    param_03_value = 5;
    expr_05_in1 = 0;
    expr_05_out1 = 0;
    param_04_value = 2;
    expr_06_in1 = 0;
    expr_06_out1 = 0;
    param_05_value = 5;
    _currentTime = 0;
    audioProcessSampleCount = 0;
    sampleOffsetIntoNextAudioBuffer = 0;
    zeroBuffer = nullptr;
    dummyBuffer = nullptr;
    signals[0] = nullptr;
    signals[1] = nullptr;
    didAllocateSignals = 0;
    vs = 0;
    maxvs = 0;
    sr = 44100;
    invsr = 0.000022675736961451248;
    pink_tilde_01_randomShift = 8;
    pink_tilde_01_numRows = 16;
    slide_01_prev = 0;
    snapshot_01_calc = 0;
    snapshot_01_nextTime = 0;
    snapshot_01_count = 0;
    snapshot_01_lastValue = 0;
    average_tilde_01_currentWindowSize = 44100;
    average_tilde_01_accum = 0;
    average_tilde_01_effectiveWindowSize = 0;
    average_tilde_01_bufferPos = 0;
    average_tilde_01_wantsReset = false;
    average_tilde_01_resetFlag = false;
    average_tilde_01_setupDone = false;
    param_01_lastValue = 0;
    param_02_lastValue = 0;
    param_03_lastValue = 0;
    param_04_lastValue = 0;
    param_05_lastValue = 0;
    globaltransport_tempo = nullptr;
    globaltransport_state = nullptr;
    stackprotect_count = 0;
    _voiceIndex = 0;
    _noteNumber = 0;
    isMuted = 1;
}

    // data ref strings
    struct DataRefStrings {
    	static constexpr auto& name0 = "pink_tilde_01_rowsobj";
    	static constexpr auto& file0 = "";
    	static constexpr auto& tag0 = "buffer~";
    	static constexpr auto& name1 = "average_tilde_01_bufferobj";
    	static constexpr auto& file1 = "";
    	static constexpr auto& tag1 = "buffer~";
    	DataRefStrings* operator->() { return this; }
    	const DataRefStrings* operator->() const { return this; }
    };

    DataRefStrings dataRefStrings;

// member variables

    number dspexpr_01_in1;
    number dspexpr_01_in2;
    number dspexpr_02_in1;
    number dspexpr_02_in2;
    number slide_01_x;
    number slide_01_up;
    number slide_01_down;
    number expr_01_in1;
    number expr_01_in2;
    number expr_01_in3;
    number expr_01_out1;
    list scale_01_input;
    number scale_01_inlow;
    number scale_01_inhigh;
    number scale_01_outlow;
    number scale_01_outhigh;
    number scale_01_power;
    list scale_01_out;
    number expr_02_in1;
    number expr_02_out1;
    number snapshot_01_interval;
    number snapshot_01_out;
    number average_tilde_01_x;
    number average_tilde_01_windowSize;
    number average_tilde_01_reset;
    number dspexpr_03_in1;
    number param_01_value;
    number expr_03_in1;
    number expr_03_in2;
    number expr_03_out1;
    number param_02_value;
    number expr_04_in1;
    number expr_04_out1;
    number param_03_value;
    number expr_05_in1;
    number expr_05_out1;
    number param_04_value;
    number expr_06_in1;
    number expr_06_out1;
    number param_05_value;
    MillisecondTime _currentTime;
    ENGINE _internalEngine;
    UInt64 audioProcessSampleCount;
    Index sampleOffsetIntoNextAudioBuffer;
    signal zeroBuffer;
    signal dummyBuffer;
    SampleValue * signals[2];
    bool didAllocateSignals;
    Index vs;
    Index maxvs;
    number sr;
    number invsr;
    Index pink_tilde_01_last;
    Int pink_tilde_01_randomShift;
    number pink_tilde_01_numRows;
    IntBufferRef pink_tilde_01_rows;
    Int pink_tilde_01_runningSum;
    Int pink_tilde_01_index;
    Int pink_tilde_01_indexMask;
    number pink_tilde_01_scalar;
    number slide_01_prev;
    number snapshot_01_calc;
    number snapshot_01_nextTime;
    SampleIndex snapshot_01_count;
    number snapshot_01_lastValue;
    Int average_tilde_01_currentWindowSize;
    number average_tilde_01_accum;
    Int average_tilde_01_effectiveWindowSize;
    Int average_tilde_01_bufferPos;
    bool average_tilde_01_wantsReset;
    bool average_tilde_01_resetFlag;
    Float64BufferRef average_tilde_01_buffer;
    bool average_tilde_01_setupDone;
    number param_01_lastValue;
    number param_02_lastValue;
    number param_03_lastValue;
    number param_04_lastValue;
    number param_05_lastValue;
    signal globaltransport_tempo;
    signal globaltransport_state;
    number stackprotect_count;
    DataRef pink_tilde_01_rowsobj;
    DataRef average_tilde_01_bufferobj;
    Index _voiceIndex;
    Int _noteNumber;
    Index isMuted;
    indexlist paramInitIndices;
    indexlist paramInitOrder;
    bool _isInitialized = false;
};

static PatcherInterface* creaternbomatic()
{
    return new rnbomatic<EXTERNALENGINE>();
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" PatcherFactoryFunctionPtr GetPatcherFactoryFunction()
#else
extern "C" PatcherFactoryFunctionPtr rnbomaticFactoryFunction()
#endif
{
    return creaternbomatic;
}

#ifndef RNBO_NO_PATCHERFACTORY
extern "C" void SetLogger(Logger* logger)
#else
void rnbomaticSetLogger(Logger* logger)
#endif
{
    console = logger;
}

} // end RNBO namespace

