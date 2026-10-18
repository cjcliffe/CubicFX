#pragma once

struct ImFont;

// Large title font loaded by applyUiStyle; null when the font file is missing.
ImFont *uiTitleFont();

void applyUiStyle();

