#include "streaming/BufferController.h"

#include <algorithm>
#include <limits>

BufferEstimate BufferController::estimate(qint64 contiguousBytes,
                                          qint64 fileSizeBytes,
                                          qint64 mediaBitrateBitsPerSecond,
                                          qint64 downloadRateBytesPerSecond,
                                          bool rebuffering)
{
    BufferEstimate result;
    result.targetSeconds = rebuffering ? RebufferTargetSeconds : InitialTargetSeconds;
    if (fileSizeBytes > 0 && contiguousBytes >= fileSizeBytes) {
        result.ready = true;
        result.playableSeconds = std::numeric_limits<double>::infinity();
        return result;
    }
    if (mediaBitrateBitsPerSecond <= 0) return result;

    const double playbackBytesPerSecond =
        static_cast<double>(mediaBitrateBitsPerSecond) / 8.0;
    result.playableSeconds = static_cast<double>(std::max<qint64>(0, contiguousBytes))
        / playbackBytesPerSecond;
    const bool rateSustainable = !rebuffering
        || static_cast<double>(downloadRateBytesPerSecond) >= playbackBytesPerSecond;
    result.ready = result.playableSeconds >= result.targetSeconds && rateSustainable;

    const double missingBytes = std::max(
        0.0, result.targetSeconds * playbackBytesPerSecond
                 - static_cast<double>(contiguousBytes));
    if (missingBytes > 0.0 && downloadRateBytesPerSecond > 0) {
        result.estimatedWaitSeconds = missingBytes
            / static_cast<double>(downloadRateBytesPerSecond);
    }
    return result;
}
