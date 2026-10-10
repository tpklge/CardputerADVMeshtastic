#pragma once

namespace cardputer_adv {
// FAT does not replace an existing destination. Keep the previous generation
// until the verified temporary file has reached its final name.
template<class Filesystem, class Path>
bool replace_file(Filesystem& fs, const Path& source, const Path& target, const Path& backup) {
    if (!fs.exists(source) || fs.exists(backup)) return false;
    const bool previous = fs.exists(target);
    if (previous && !fs.rename(target, backup)) return false;
    if (!fs.rename(source, target)) {
        if (previous) fs.rename(backup, target);
        return false;
    }
    return !previous || fs.remove(backup);
}
}
