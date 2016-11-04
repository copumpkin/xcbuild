/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <libutil/MemoryFilesystem.h>
#include <libutil/FSUtil.h>

#include <algorithm>

#include <cassert>

using libutil::MemoryFilesystem;
using libutil::FSUtil;

MemoryFilesystem::Entry::
Entry(std::string const &name, Type type) :
    _name(name),
    _type(type)
{
}

MemoryFilesystem::Entry *MemoryFilesystem::Entry::
child(std::string const &name)
{
    assert(_type == MemoryFilesystem::Entry::Type::Directory);

    for (MemoryFilesystem::Entry &entry : _children) {
        if (entry.name() == name) {
            return &entry;
        }
    }

    return nullptr;
}

MemoryFilesystem::Entry const *MemoryFilesystem::Entry::
child(std::string const &name) const
{
    assert(_type == MemoryFilesystem::Entry::Type::Directory);

    for (MemoryFilesystem::Entry const &entry : _children) {
        if (entry.name() == name) {
            return &entry;
        }
    }

    return nullptr;
}

MemoryFilesystem::Entry MemoryFilesystem::Entry::
File(std::string const &name, std::vector<uint8_t> const &contents)
{
    MemoryFilesystem::Entry entry = MemoryFilesystem::Entry(name, MemoryFilesystem::Entry::Type::File);
    entry.contents() = contents;
    return entry;
}

MemoryFilesystem::Entry MemoryFilesystem::Entry::
Directory(std::string const &name, std::vector<Entry> const &children)
{
    MemoryFilesystem::Entry entry = MemoryFilesystem::Entry(name, MemoryFilesystem::Entry::Type::Directory);
    entry.children() = children;
    return entry;
}

MemoryFilesystem::
MemoryFilesystem(std::vector<MemoryFilesystem::Entry> const &entries) :
    _root(MemoryFilesystem::Entry::Directory("/", entries))
{
}

template<typename T, typename U, typename V>
static bool
WalkPath(
    U filesystem,
    std::string const &path,
    bool all,
    V const &cb)
{
    /* All paths are expected to be absolute. */
    if (path.front() != '/') {
        return false;
    }

    std::string normalized = FSUtil::NormalizePath(path);
    if (normalized.empty()) {
        return false;
    }

    T *current = &filesystem->root();
    assert(current->type() == MemoryFilesystem::Entry::Type::Directory);

    std::string::size_type start = 0;
    std::string::size_type end = normalized.find('/', start);

    do {
        bool final = (end == std::string::npos);

        T *next = current;
        std::string name = normalized.substr(start, end - start);
        if (!name.empty()) {
            /* Get the next path component. */
            next = current->child(name);
        }

        if (all || final) {
            /* Call back with information. */
            next = cb(current, name, next);
        }

        if (final) {
            /* Nothing more. */
            return (next != nullptr);
        }

        /* Path component not found or not directory. */
        if (next == nullptr || next->type() != MemoryFilesystem::Entry::Type::Directory) {
            return false;
        }

        current = next;

        /* Move to next path component. */
        start = end + 1;
        end = normalized.find('/', start);
    } while (true);
}

bool MemoryFilesystem::
exists(std::string const &path) const
{
    return WalkPath<MemoryFilesystem::Entry const>(this, path, false, [](MemoryFilesystem::Entry const *parent, std::string const &name, MemoryFilesystem::Entry const *entry) {
        /* Nothing to do. */
        return entry;
    });
}

bool MemoryFilesystem::
isReadable(std::string const &path) const
{
    return this->exists(path);
}

bool MemoryFilesystem::
isWritable(std::string const &path) const
{
    return this->exists(path);
}

bool MemoryFilesystem::
isExecutable(std::string const &path) const
{
    return this->exists(path);
}

bool MemoryFilesystem::
isFile(std::string const &path) const
{
    return WalkPath<MemoryFilesystem::Entry const>(this, path, false, [](MemoryFilesystem::Entry const *parent, std::string const &name, MemoryFilesystem::Entry const *entry) -> MemoryFilesystem::Entry const * {
        if (entry != nullptr && entry->type() != MemoryFilesystem::Entry::Type::File) {
            return nullptr;
        }

        return entry;
    });
}

bool MemoryFilesystem::
createFile(std::string const &path)
{
    return WalkPath<MemoryFilesystem::Entry>(this, path, false, [](MemoryFilesystem::Entry *parent, std::string const &name, MemoryFilesystem::Entry *entry) -> MemoryFilesystem::Entry * {
        if (entry != nullptr) {
            if (entry->type() == MemoryFilesystem::Entry::Type::File) {
                /* Exists as a file. */
                return entry;
            } else {
                /* Exists already, but not as a file. */
                return nullptr;
            }
        } else {
            /* Add empty file. */
            MemoryFilesystem::Entry file = MemoryFilesystem::Entry::File(name, std::vector<uint8_t>());
            std::vector<MemoryFilesystem::Entry> *children = &parent->children();
            children->emplace_back(std::move(file));
            return &children->back();
        }
    });
}

bool MemoryFilesystem::
read(std::vector<uint8_t> *contents, std::string const &path, size_t offset, ext::optional<size_t> length) const
{
    return WalkPath<MemoryFilesystem::Entry const>(this, path, false, [&](MemoryFilesystem::Entry const *parent, std::string const &name, MemoryFilesystem::Entry const *entry) -> MemoryFilesystem::Entry const * {
        if (entry == nullptr || entry->type() != MemoryFilesystem::Entry::Type::File) {
            return nullptr;
        }

        if (offset == 0 && !length) {
            *contents = entry->contents();
        } else {
            std::vector<uint8_t> const &from = entry->contents();
            size_t end = (length ? offset + *length : from.size() - offset);
            *contents = std::vector<uint8_t>(from.begin() + offset, from.begin() + end);
        }

        return entry;
    });
}

bool MemoryFilesystem::
write(std::vector<uint8_t> const &contents, std::string const &path)
{
    return WalkPath<MemoryFilesystem::Entry>(this, path, false, [&](MemoryFilesystem::Entry *parent, std::string const &name, MemoryFilesystem::Entry *entry) -> MemoryFilesystem::Entry * {
        if (entry != nullptr) {
            if (entry->type() == MemoryFilesystem::Entry::Type::File) {
                /* Exists as a file, replace contents. */
                entry->contents() = contents;
                return entry;
            } else {
                /* Exists already, but not as a file. */
                return nullptr;
            }
        } else {
            /* Add file. */
            MemoryFilesystem::Entry file = MemoryFilesystem::Entry::File(name, contents);
            std::vector<MemoryFilesystem::Entry> *children = &parent->children();
            children->emplace_back(std::move(file));
            return &children->back();
        }
    });
}

bool MemoryFilesystem::
copyFile(std::string const &from, std::string const &to)
{
    return Filesystem::copyFile(from, to);
}

bool MemoryFilesystem::
removeFile(std::string const &path)
{
    return WalkPath<MemoryFilesystem::Entry>(this, path, false, [](MemoryFilesystem::Entry *parent, std::string const &name, MemoryFilesystem::Entry *entry) -> MemoryFilesystem::Entry * {
        if (entry != nullptr) {
            if (entry->type() == MemoryFilesystem::Entry::Type::File) {
                /* Found, remove it. */
                std::vector<MemoryFilesystem::Entry> *children = &parent->children();
                children->erase(std::remove_if(children->begin(), children->end(), [&](MemoryFilesystem::Entry const &entry) {
                    return (entry.name() == name);
                }), children->end());
                return parent;
            } else {
                /* Can't remove directories. */
                return nullptr;
            }
        } else {
            /* Did not exist. */
            return nullptr;
        }
    });
}

bool MemoryFilesystem::
isSymbolicLink(std::string const &path) const
{
    return false;
}

ext::optional<std::string> MemoryFilesystem::
readSymbolicLink(std::string const &path) const
{
    return ext::nullopt;
}

bool MemoryFilesystem::
writeSymbolicLink(std::string const &target, std::string const &path)
{
    return false;
}

bool MemoryFilesystem::
copySymbolicLink(std::string const &from, std::string const &to)
{
    return false;
}

bool MemoryFilesystem::
removeSymbolicLink(std::string const &path)
{
    return false;
}

bool MemoryFilesystem::
isDirectory(std::string const &path) const
{
    return WalkPath<MemoryFilesystem::Entry const>(this, path, false, [](MemoryFilesystem::Entry const *parent, std::string const &name, MemoryFilesystem::Entry const *entry) -> MemoryFilesystem::Entry const * {
        if (entry != nullptr && entry->type() != MemoryFilesystem::Entry::Type::Directory) {
            return nullptr;
        }

        return entry;
    });
}

bool MemoryFilesystem::
createDirectory(std::string const &path, bool recursive)
{
    return WalkPath<MemoryFilesystem::Entry>(this, path, recursive, [](MemoryFilesystem::Entry *parent, std::string const &name, MemoryFilesystem::Entry *entry) -> MemoryFilesystem::Entry * {
        if (entry != nullptr) {
            if (entry->type() == MemoryFilesystem::Entry::Type::Directory) {
                /* Intermediate directory already exists. */
                return entry;
            } else {
                /* Exists already, but not as a directory. */
                return nullptr;
            }
        } else {
            /* Add intermediate directory. */
            MemoryFilesystem::Entry directory = MemoryFilesystem::Entry::Directory(name, { });
            std::vector<MemoryFilesystem::Entry> *children = &parent->children();
            children->emplace_back(std::move(directory));
            return &children->back();
        }
    });
}

bool MemoryFilesystem::
copyDirectory(std::string const &from, std::string const &to, bool recursive)
{
    return Filesystem::copyDirectory(from, to, recursive);
}

bool MemoryFilesystem::
readDirectory(std::string const &path, bool recursive, std::function<void(std::string const &)> const &cb) const
{
    std::function<void(ext::optional<std::string> const &, MemoryFilesystem::Entry const *)> process =
        [&path, &recursive, &cb, &process](ext::optional<std::string> const &subpath, MemoryFilesystem::Entry const *entry) {
        /* Report children. */
        for (MemoryFilesystem::Entry const &child : entry->children()) {
            std::string path = (subpath ? *subpath + "/" + child.name() : child.name());
            cb(path);
        }

        /* Process subdirectories. */
        if (recursive) {
            for (MemoryFilesystem::Entry const &child : entry->children()) {
                if (child.type() == MemoryFilesystem::Entry::Type::Directory) {
                    std::string path = (subpath ? *subpath + "/" + child.name() : child.name());
                    process(path, &child);
                }
            }
        }
    };

    return WalkPath<MemoryFilesystem::Entry const>(this, path, false, [&process](MemoryFilesystem::Entry const *parent, std::string const &name, MemoryFilesystem::Entry const *entry) -> MemoryFilesystem::Entry const * {
        if (entry != nullptr && entry->type() == MemoryFilesystem::Entry::Type::Directory) {
            /* Found directory, process. */
            process(ext::nullopt, entry);
            return entry;
        } else {
            /* Did not exit or not a directory. */
            return nullptr;
        }
    });
}

bool MemoryFilesystem::
removeDirectory(std::string const &path, bool recursive)
{
    return WalkPath<MemoryFilesystem::Entry>(this, path, false, [&recursive](MemoryFilesystem::Entry *parent, std::string const &name, MemoryFilesystem::Entry *entry) -> MemoryFilesystem::Entry * {
        if (entry != nullptr && entry->type() == MemoryFilesystem::Entry::Type::Directory) {
            /* Only remove empty directories unless recursive. */
            if (!recursive && !entry->children().empty()) {
                return nullptr;
            }

            /* Remove directory. */
            std::vector<MemoryFilesystem::Entry> *children = &parent->children();
            children->erase(std::remove_if(children->begin(), children->end(), [&](MemoryFilesystem::Entry const &entry) {
                return (entry.name() == name);
            }), children->end());
            return parent;
        } else {
            /* Did not exist or not a directory. */
            return nullptr;
        }
    });
}

std::string MemoryFilesystem::
resolvePath(std::string const &path) const
{
    if (exists(path)) {
        return FSUtil::NormalizePath(path);
    } else {
        return std::string();
    }
}

