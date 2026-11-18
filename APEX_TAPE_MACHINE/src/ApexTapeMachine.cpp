// APEX TAPE MACHINE — stereo tape behavioral model + acoustic echo chamber.
// SPDX-License-Identifier: LicenseRef-Proprietary
// DSP is original APEX code. No third-party DSP dependencies.
//
// Signal path: input gain -> tape saturation/compensation/response/motion/noise
// -> crosstalk -> acoustic chamber send/early-reflection/feedback -> output gain.
//
// Realtime discipline: all delay buffers allocated in prepare(); no allocations,
// locks, file I/O, logging, or transcendental functions in the sample loop.

#include "mpc_plugin.h"
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr int kNumChannels = 2;

enum Param {
    kInput = 0, kOutput, kDrive, kTapeMix, kBias, kComp, kSpeed, kFormula,
    kWow, kFlutter, kHiss, kCrosstalk, kHfShape, kTapeOn,
    kChamberOn, kSend, kChamberMix, kSize, kDecay, kDamping, kPreDelay, kWidth,
    kNumParams
};

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float gainDb(float db) { return std::pow(10.0f, db * 0.05f); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float softClip(float x) {
    const float ax = std::fabs(x);
    return x / (1.0f + 0.24f * ax);
}
struct Smooth {
    float v = 1.0f;
    float a = 0.0025f;
    void snap(float x) { v = x; }
    float next(float x) { v += a * (x - v); return v; }
};
struct TapeChannel {
    float env = 0.0f, low = 0.0f;
    float wowSin = 0.0f, wowCos = 1.0f, wowStepSin = 0.0f, wowStepCos = 1.0f;
    float flutterSin = 0.0f, flutterCos = 1.0f, flutterStepSin = 0.0f, flutterStepCos = 1.0f;
    float noiseLP = 0.0f;
};
}

class ApexTapeMachine : public mpc::Plugin {
public:
    ApexTapeMachine() : mpc::Plugin(makeInfo()) {
        // Parameter 0 is harmless under the MPC host's prepare-time parameter nudge.
        addParam(mpc::floatParam("input", "Input", -18.0f, 18.0f, 0.0f, "dB", 1));
        addParam(mpc::floatParam("output", "Output", -18.0f, 18.0f, 0.0f, "dB", 1));
        addParam(mpc::floatParam("drive", "Drive", 0.0f, 100.0f, 38.0f, "%", 0));
        addParam(mpc::floatParam("tapemix", "Tape Mix", 0.0f, 100.0f, 100.0f, "%", 0));
        addParam(mpc::floatParam("bias", "Bias", 0.0f, 100.0f, 52.0f, "%", 0));
        addParam(mpc::floatParam("comp", "Comp", 0.0f, 100.0f, 28.0f, "%", 0));
        addParam(mpc::choiceParam("speed", "Speed", {"15 IPS", "30 IPS"}, 1));
        addParam(mpc::choiceParam("formula", "Formula", {"456", "900", "GP9"}, 0));
        addParam(mpc::floatParam("wow", "Wow", 0.0f, 100.0f, 3.0f, "%", 0));
        addParam(mpc::floatParam("flutter", "Flutter", 0.0f, 100.0f, 2.0f, "%", 0));
        addParam(mpc::floatParam("hiss", "Hiss", 0.0f, 100.0f, 0.0f, "%", 0));
        addParam(mpc::floatParam("crosstalk", "Crosstalk", 0.0f, 100.0f, 8.0f, "%", 0));
        addParam(mpc::floatParam("hfshape", "HF Shape", 0.0f, 100.0f, 50.0f, "%", 0));
        addParam(mpc::boolParam("tapeon", "Tape On", true));

        addParam(mpc::boolParam("chamberon", "Chamber", true));
        addParam(mpc::floatParam("send", "Send", 0.0f, 100.0f, 35.0f, "%", 0));
        addParam(mpc::floatParam("chambermix", "Mix", 0.0f, 100.0f, 0.0f, "%", 0));
        addParam(mpc::floatParam("size", "Size", 0.0f, 100.0f, 55.0f, "%", 0));
        addParam(mpc::floatParam("decay", "Decay", 0.0f, 100.0f, 55.0f, "%", 0));
        addParam(mpc::floatParam("damping", "Damping", 0.0f, 100.0f, 45.0f, "%", 0));
        addParam(mpc::floatParam("predelay", "PreDelay", 0.0f, 240.0f, 18.0f, "ms", 1));
        addParam(mpc::floatParam("width", "Width", 0.0f, 100.0f, 80.0f, "%", 0));

        addProgram("90s Hip-Hop", {2, 0, 48, 100, 52, 36, 0, 0, 5, 3, 2, 10, 48, 1, 1, 38, 12, 54, 58, 42, 22, 84});
        addProgram("Dusty Boom Bap", {3, -1, 60, 100, 48, 42, 0, 0, 12, 8, 18, 18, 35, 1, 1, 28, 10, 42, 62, 58, 28, 72});
        addProgram("Drum Bus Glue", {1, -1, 42, 100, 56, 45, 1, 1, 1, 2, 0, 6, 62, 1, 0, 0, 0, 45, 40, 46, 10, 70});
        addProgram("Thick Bass", {2, -2, 34, 100, 55, 32, 1, 2, 1, 1, 0, 3, 58, 1, 0, 0, 0, 45, 42, 55, 8, 45});
        addProgram("Vocal Chamber", {0, 0, 34, 100, 50, 18, 1, 0, 2, 1, 1, 4, 55, 1, 1, 48, 26, 70, 68, 38, 34, 96});
        addProgram("Hot Tape Print", {7, -2, 76, 100, 60, 58, 0, 2, 8, 5, 5, 12, 38, 1, 1, 22, 8, 40, 54, 62, 14, 68});
        addProgram("Open Reel Master", {0, 0, 22, 100, 55, 12, 1, 2, 0, 0, 0, 4, 68, 1, 0, 0, 0, 40, 35, 55, 10, 75});
        addProgram("DJ Mixtape", {3, -1, 52, 100, 44, 44, 0, 0, 9, 7, 12, 16, 34, 1, 1, 40, 16, 60, 64, 55, 24, 88});
        addProgram("APEX Default", {0, 0, 38, 100, 52, 28, 1, 0, 3, 2, 0, 8, 50, 1, 1, 35, 0, 55, 55, 45, 18, 80});
    }

    void prepare(double sampleRate, int /*maxBlockSize*/) override {
        sr_ = static_cast<float>(sampleRate);
        // ~220 ms chamber loop plus fixed pre-delay. Allocate only here.
        const std::size_t n = static_cast<std::size_t>(sr_ * 0.22f) + 8;
        delayL_.assign(n, 0.0f); delayR_.assign(n, 0.0f);
        const std::size_t pn = static_cast<std::size_t>(sr_ * 0.25f) + 8;
        preL_.assign(pn, 0.0f); preR_.assign(pn, 0.0f);
        const std::size_t mn = static_cast<std::size_t>(sr_ * 0.012f) + 8;
        modL_.assign(mn, 0.0f); modR_.assign(mn, 0.0f);
        modWriteL_ = modWriteR_ = 0;
        write_ = preWrite_ = 0;
        reset();
    }

    void reset() override {
        tape_[0] = TapeChannel{}; tape_[1] = TapeChannel{};
        std::fill(delayL_.begin(), delayL_.end(), 0.0f);
        std::fill(delayR_.begin(), delayR_.end(), 0.0f);
        std::fill(preL_.begin(), preL_.end(), 0.0f);
        std::fill(preR_.begin(), preR_.end(), 0.0f);
        std::fill(modL_.begin(), modL_.end(), 0.0f);
        std::fill(modR_.begin(), modR_.end(), 0.0f);
        modWriteL_ = modWriteR_ = 0;
        write_ = preWrite_ = 0; dampL_ = dampR_ = 0.0f;
        in_.snap(gainDb(get(kInput))); out_.snap(gainDb(get(kOutput)));
        tapeMix_.snap(get(kTapeMix) * 0.01f);
        chamberMix_.snap(get(kChamberMix) * 0.01f);
    }

    void process(mpc::ProcessContext& ctx) override {
        const float targetIn = gainDb(get(kInput));
        const float targetOut = gainDb(get(kOutput));
        const float targetTapeMix = get(kTapeMix) * 0.01f;
        const float targetChamberMix = get(kChamberMix) * 0.01f;
        const float drive = get(kDrive) * 0.01f;
        const float bias = get(kBias) * 0.01f;
        const float comp = get(kComp) * 0.01f;
        const float wow = get(kWow) * 0.01f;
        const float flutter = get(kFlutter) * 0.01f;
        const float hiss = get(kHiss) * 0.01f;
        const float cross = get(kCrosstalk) * 0.01f * 0.065f;
        const float hf = get(kHfShape) * 0.01f;
        const int speed = getInt(kSpeed);
        const int formula = getInt(kFormula);
        const bool tapeOn = getBool(kTapeOn);
        const bool chamberOn = getBool(kChamberOn);
        const float send = get(kSend) * 0.01f;
        const float size = get(kSize) * 0.01f;
        const float decay = get(kDecay) * 0.01f;
        const float damping = get(kDamping) * 0.01f;
        const float predelay = get(kPreDelay) * 0.001f;
        const float width = get(kWidth) * 0.01f;

        const float formulaHeadroom = formula == 0 ? 0.94f : (formula == 1 ? 1.07f : 1.17f);
        const float cutoff = (speed == 0 ? 14500.0f : 19000.0f) * (0.76f + hf * 0.24f);
        const float lp = 1.0f - std::exp(-kTwoPi * std::min(cutoff, sr_ * 0.44f) / sr_);
        const float wowHz = speed == 0 ? 0.17f : 0.23f;
        const float flutterHz = speed == 0 ? 5.2f : 6.7f;
        setLfoSteps(tape_[0], wowHz, flutterHz);
        setLfoSteps(tape_[1], wowHz, flutterHz);
        const float feedback = 0.16f + decay * 0.76f;
        const float dampCoef = 0.035f + damping * 0.68f;
        const std::size_t loopN = delayL_.size();
        const std::size_t preN = preL_.size();
        const std::size_t preSamples = std::min<std::size_t>(static_cast<std::size_t>(predelay * sr_), preN - 1);

        for (int i = 0; i < ctx.numFrames; ++i) {
            const float inputGain = in_.next(targetIn);
            const float outputGain = out_.next(targetOut);
            const float tm = tapeMix_.next(targetTapeMix);
            const float cm = chamberMix_.next(targetChamberMix);
            const float dryL = (ctx.inputs && ctx.inputs[0]) ? ctx.inputs[0][i] : 0.0f;
            const float dryR = (ctx.inputs && ctx.numInputs > 1 && ctx.inputs[1]) ? ctx.inputs[1][i] : dryL;
            float l = dryL * inputGain, r = dryR * inputGain;

            if (tapeOn) {
                float inL = l + tape_[1].prev * cross;
                float inR = r + tape_[0].prev * cross;
                float wetL = tapeSample(inL, tape_[0], modL_, modWriteL_, drive, bias, comp, formulaHeadroom, lp, wow, flutter, speed == 0);
                float wetR = tapeSample(inR, tape_[1], modR_, modWriteR_, drive, bias, comp, formulaHeadroom, lp, wow, flutter, speed == 0);
                // Independent pseudo-random generators keep stereo noise decorrelated.
                wetL += noiseSample(hiss, noiseL_);
                wetR += noiseSample(hiss, noiseR_);
                l = lerp(l, wetL, tm); r = lerp(r, wetR, tm);
            }

            if (chamberOn && cm > 0.0001f && loopN > 16) {
                const std::size_t preRead = (preWrite_ + preN - preSamples) % preN;
                const float preInL = preL_[preRead], preInR = preR_[preRead];
                preL_[preWrite_] = l * send; preR_[preWrite_] = r * send;
                preWrite_ = (preWrite_ + 1) % preN;

                const std::size_t tapA = std::max<std::size_t>(1, loopN / (5 + static_cast<int>(size * 6)));
                const std::size_t tapB = std::max<std::size_t>(1, loopN / (9 + static_cast<int>(size * 9)));
                const float earlyL = delayL_[(write_ + loopN - tapA) % loopN] * 0.22f
                                   + delayR_[(write_ + loopN - tapB) % loopN] * 0.15f;
                const float earlyR = delayR_[(write_ + loopN - tapA) % loopN] * 0.22f
                                   + delayL_[(write_ + loopN - tapB) % loopN] * 0.15f;
                const float readL = delayL_[write_], readR = delayR_[write_];
                dampL_ += dampCoef * (readL - dampL_);
                dampR_ += dampCoef * (readR - dampR_);
                delayL_[write_] = preInL + (dampL_ * 0.58f + dampR_ * 0.22f) * feedback;
                delayR_[write_] = preInR + (dampR_ * 0.58f + dampL_ * 0.22f) * feedback;
                write_ = (write_ + 1) % loopN;
                const float mid = 0.5f * (dampL_ + dampR_ + earlyL + earlyR);
                const float side = 0.5f * (dampL_ + earlyL - dampR_ - earlyR) * width;
                const float wetL = mid + side;
                const float wetR = mid - side;
                l = lerp(l, softClip(l + wetL * 0.75f), cm);
                r = lerp(r, softClip(r + wetR * 0.75f), cm);
            }
            ctx.outputs[0][i] = softClip(l * outputGain);
            if (ctx.numOutputs > 1 && ctx.outputs[1]) ctx.outputs[1][i] = softClip(r * outputGain);
        }
    }

private:
    static mpc::PluginInfo makeInfo() {
        mpc::PluginInfo i;
        i.name = "APEX Tape Machine";
        i.vendor = "APEX Audio Labs";
        i.uniqueId = MPC_FOURCC('A','t','M','1');
        i.version = 100;
        i.isInstrument = false; i.numInputs = 2; i.numOutputs = 2;
        i.tailSamples = 0;
        return i;
    }
    void setLfoSteps(TapeChannel& s, float wowHz, float flutterHz) {
        const float w = kTwoPi * wowHz / sr_;
        const float f = kTwoPi * flutterHz / sr_;
        s.wowStepSin = std::sin(w); s.wowStepCos = std::cos(w);
        s.flutterStepSin = std::sin(f); s.flutterStepCos = std::cos(f);
    }
    float tapeSample(float x, TapeChannel& s, std::vector<float>& modBuf, std::size_t& modWrite,
                     float drive, float bias, float comp, float headroom, float lpCoef,
                     float wow, float flutter, bool slowSpeed) {
        const float pre = 1.0f + drive * 7.0f;
        const float offset = (bias - 0.5f) * 0.14f;
        const float z = std::tanh((x * pre + offset) / headroom);
        const float zero = std::tanh(offset / headroom);
        float sat = (z - zero) / std::max(0.25f, 1.0f - std::fabs(zero));
        s.low += lpCoef * (sat - s.low);
        sat = lerp(sat, s.low, slowSpeed ? 0.14f : 0.025f);
        const float mag = std::fabs(sat);
        const float envCoef = mag > s.env ? 0.004f : 0.0005f;
        s.env += envCoef * (mag - s.env);
        sat /= (1.0f + comp * std::max(0.0f, s.env - 0.22f) * 2.4f);

        // True variable-delay wow/flutter: changing tape speed moves the read head.
        const float delayMs = 4.2f + s.wowSin * wow * 0.95f + s.flutterSin * flutter * 0.22f;
        const float delaySamples = clampf(delayMs * 0.001f * sr_, 1.0f, static_cast<float>(modBuf.size() - 3));
        modBuf[modWrite] = sat;
        float readPos = static_cast<float>(modWrite) - delaySamples;
        const float len = static_cast<float>(modBuf.size());
        while (readPos < 0.0f) readPos += len;
        while (readPos >= len) readPos -= len;
        const std::size_t i0 = static_cast<std::size_t>(readPos);
        const std::size_t i1 = (i0 + 1) % modBuf.size();
        const float moved = lerp(modBuf[i0], modBuf[i1], readPos - static_cast<float>(i0));
        modWrite = (modWrite + 1) % modBuf.size();

        const float wSin = s.wowSin * s.wowStepCos + s.wowCos * s.wowStepSin;
        const float wCos = s.wowCos * s.wowStepCos - s.wowSin * s.wowStepSin;
        s.wowSin = wSin; s.wowCos = wCos;
        const float fSin = s.flutterSin * s.flutterStepCos + s.flutterCos * s.flutterStepSin;
        const float fCos = s.flutterCos * s.flutterStepCos - s.flutterSin * s.flutterStepSin;
        s.flutterSin = fSin; s.flutterCos = fCos;
        return moved;
    }
    float noiseSample(float amount, uint32_t& state) {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        const float white = (static_cast<float>(state & 0x00FFFFFFu) / 8388607.5f - 1.0f) * amount * 0.00065f;
        return white;
    }
    static float nextNoiseSeed(uint32_t& state) {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5; return static_cast<float>(state & 0x00FFFFFFu);
    }

    float sr_ = 44100.0f;
    TapeChannel tape_[2]{};
    Smooth in_{}, out_{}, tapeMix_{}, chamberMix_{};
    std::vector<float> delayL_, delayR_, preL_, preR_, modL_, modR_;
    std::size_t write_ = 0, preWrite_ = 0, modWriteL_ = 0, modWriteR_ = 0;
    float dampL_ = 0.0f, dampR_ = 0.0f;
    uint32_t noiseL_ = 0x1234567u, noiseR_ = 0x7654321u;
};

MPC_PLUGIN_ENTRY(ApexTapeMachine)
