#include "note_store.h"

#include <algorithm>

namespace {
bool isBetterNote(const Note &lhs, const Note &rhs) {
    if (lhs.updatedUtc != rhs.updatedUtc) {
        return lhs.updatedUtc > rhs.updatedUtc;
    }
    if (lhs.createdUtc != rhs.createdUtc) {
        return lhs.createdUtc > rhs.createdUtc;
    }
    return lhs.id < rhs.id;
}
}  // namespace

bool MemoryNoteBackend::load(std::vector<Note> &notes) const {
    notes = _notes;
    return true;
}

bool MemoryNoteBackend::save(const std::vector<Note> &notes) {
    _notes = notes;
    return true;
}

NoteStore::NoteStore(INoteBackend &backend) : _backend(backend) {
    _backend.load(_notes);
    for (const Note &note : _notes) {
        if (note.id.rfind("note-", 0) == 0) {
            const uint32_t value = static_cast<uint32_t>(std::strtoul(note.id.c_str() + 5, nullptr, 10));
            _nextId = std::max(_nextId, value + 1);
        }
    }
    trimToLimit();
}

Note NoteStore::upsert(Note note) {
    if (note.id.empty()) {
        note.id = "note-" + std::to_string(_nextId++);
    }
    if (note.createdUtc == 0) {
        note.createdUtc = note.updatedUtc;
    }

    auto it = std::find_if(_notes.begin(), _notes.end(), [&](const Note &existing) {
        return existing.id == note.id;
    });
    if (it != _notes.end()) {
        *it = note;
    } else {
        _notes.push_back(note);
    }
    trimToLimit();
    persist();
    return note;
}

bool NoteStore::remove(const std::string &id) {
    const size_t before = _notes.size();
    _notes.erase(std::remove_if(_notes.begin(), _notes.end(), [&](const Note &existing) {
        return existing.id == id;
    }), _notes.end());
    if (_notes.size() == before) {
        return false;
    }
    persist();
    return true;
}

std::vector<Note> NoteStore::list() const {
    std::vector<Note> notes = _notes;
    std::sort(notes.begin(), notes.end(), isBetterNote);
    return notes;
}

void NoteStore::trimToLimit() {
    if (_notes.size() <= 50) {
        return;
    }
    std::sort(_notes.begin(), _notes.end(), isBetterNote);
    _notes.resize(50);
}

void NoteStore::persist() {
    _backend.save(_notes);
}
