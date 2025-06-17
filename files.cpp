void FileExplorer::handleUndo() {
    if (currentUndoManager) {
        auto [op, valid] = currentUndoManager->undo();
        if (valid) {
            applyOperation(op, true);
            saveUndoRedoState();
            saveCurrentFile(); // Save after undo
            gEditorGit.clearFileTracking(currentFile); // Clear existing tracking
            gEditorGit.setCurrentFile(currentFile); // Re-initialize tracking
        }
    }
}

void FileExplorer::handleRedo() {
    if (currentUndoManager) {
        auto [op, valid] = currentUndoManager->redo();
        if (valid) {
            applyOperation(op, false);
            saveUndoRedoState();
            saveCurrentFile(); // Save after redo
            gEditorGit.clearFileTracking(currentFile); // Clear existing tracking
            gEditorGit.setCurrentFile(currentFile); // Re-initialize tracking
        }
    }
} 