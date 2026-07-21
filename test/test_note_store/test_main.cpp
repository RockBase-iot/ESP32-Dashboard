#include <unity.h>

#include "app/notes/note_store.h"
#include "app/notes/note_store.cpp"

void test_note_store_caps_at_fifty_notes() {
    MemoryNoteBackend backend;
    NoteStore store(backend);
    for (int i = 0; i < 55; ++i) {
        Note note;
        note.title = "N" + std::to_string(i);
        note.createdUtc = i;
        note.updatedUtc = i;
        store.upsert(note);
    }

    TEST_ASSERT_EQUAL_UINT32(50, store.list().size());
}

void test_note_delete_only_removes_local_note() {
    MemoryNoteBackend backend;
    NoteStore store(backend);
    Note note;
    note.id = "note-a";
    note.title = "Milk";
    store.upsert(note);

    TEST_ASSERT_TRUE(store.remove("note-a"));
    TEST_ASSERT_EQUAL_UINT32(0, store.list().size());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_note_store_caps_at_fifty_notes);
    RUN_TEST(test_note_delete_only_removes_local_note);
    UNITY_END();
}

void loop() {}
