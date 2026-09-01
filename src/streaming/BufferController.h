#pragma once

#include <QtGlobal>

struct BufferEstimate final
{
    double playableSeconds = 0.0;
    double targetSeconds = 30.0;
    double estimatedWaitSeconds = 0.0;
    bool ready = false;
};

class BufferController final
{
public:
    static constexpr double InitialTargetSeconds = 30.0;
    static constexpr double RebufferTargetSeconds = 10.0;

    [[nodiscard]] static BufferEstimate estimate(qint64 contiguousBytes,
                                                 qint64 fileSizeBytes,
                                                 qint64 mediaBitrateBitsPerSecond,
                                                 qint64 downloadRateBytesPerSecond,
                                                 bool rebuffering = false);
};
