#pragma once

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

struct Note {
    std::string id;
    std::string title;
    std::string body;
    int64_t createdUtc = 0;
    int64_t updatedUtc = 0;
};

class INoteBackend {
public:
    virtual ~INoteBackend() = default;
    virtual bool load(std::vector<Note> &notes) const = 0;
    virtual bool save(const std::vector<Note> &notes) = 0;
};

class MemoryNoteBackend final : public INoteBackend {
public:
    bool load(std::vector<Note> &notes) const override;
    bool save(const std::vector<Note> &notes) override;

private:
    std::vector<Note> _notes;
};

class NoteStore {
public:
    explicit NoteStore(INoteBackend &backend);

    Note upsert(Note note);
    bool remove(const std::string &id);
    std::vector<Note> list() const;

private:
    void trimToLimit();
    void persist();

    INoteBackend &_backend;
    std::vector<Note> _notes;
    uint32_t _nextId = 1;
};
