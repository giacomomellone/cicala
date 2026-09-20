#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef CICALA_MAX_QUESTIONS
#define CICALA_MAX_QUESTIONS 512
#endif
#ifndef CICALA_RECENT_RING
#define CICALA_RECENT_RING 20
#endif
#ifndef CICALA_MAX_QUESTION_BYTES
#define CICALA_MAX_QUESTION_BYTES 128
#endif
#ifndef CICALA_TEXTURE
#define CICALA_TEXTURE 1
#endif

namespace cicala
{
constexpr uint16_t kMaxQuestions = CICALA_MAX_QUESTIONS;
constexpr uint8_t kRecentRing = CICALA_RECENT_RING;
constexpr size_t kMaxQuestionBytes = CICALA_MAX_QUESTION_BYTES;
constexpr size_t kMaxCorpusBytes = 32768;
constexpr size_t kMaxManifestBytes = 2048;
constexpr bool kTexture = CICALA_TEXTURE != 0;
constexpr char kCoreVersion[] = "0.1.0";
constexpr char kSyncContractVersion[] = "0.2.0";
static_assert(CICALA_MAX_QUESTIONS > 0 && CICALA_MAX_QUESTIONS <= 65535);
static_assert(CICALA_RECENT_RING > 0 && CICALA_RECENT_RING <= 255);
static_assert(CICALA_MAX_QUESTION_BYTES > 0 && CICALA_MAX_QUESTION_BYTES <= 65535);
} // namespace cicala
