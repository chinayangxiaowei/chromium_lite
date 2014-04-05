/*
 * Copyright 2011 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "third_party/sfntly/src/subsetter/subsetter_impl.h"

#include <string.h>

#include <map>
#include <set>

#include "third_party/sfntly/src/sfntly/glyph_table.h"
#include "third_party/sfntly/src/sfntly/loca_table.h"
#include "third_party/sfntly/src/sfntly/name_table.h"
#include "third_party/sfntly/src/sfntly/tag.h"
#include "third_party/sfntly/src/sfntly/data/memory_byte_array.h"
#include "third_party/sfntly/src/sfntly/port/memory_output_stream.h"

namespace sfntly {

void ConstructName(UChar* name_part, UnicodeString* name, int32_t name_id) {
  switch(name_id) {
    case NameId::kFullFontName:
      *name = name_part;
      break;
    case NameId::kFontFamilyName:
    case NameId::kPreferredFamily:
    case NameId::kWWSFamilyName: {
      UnicodeString original = *name;
      *name = name_part;
      *name += original;
      break;
    }
    case NameId::kFontSubfamilyName:
    case NameId::kPreferredSubfamily:
    case NameId::kWWSSubfamilyName:
      *name += name_part;
      break;
    default:
      // This name part is not used to construct font name (e.g. copyright).
      // Simply ignore it.
      break;
  }
}

int32_t HashCode(int32_t platform_id, int32_t encoding_id, int32_t language_id,
                 int32_t name_id) {
  int32_t result = platform_id << 24 | encoding_id << 16 | language_id << 8;
  if (name_id == NameId::kFullFontName) {
    result |= 0xff;
  } else if (name_id == NameId::kPreferredFamily ||
             name_id == NameId::kPreferredSubfamily) {
    result |= 0xf;
  } else if (name_id == NameId::kWWSFamilyName ||
             name_id == NameId::kWWSSubfamilyName) {
    result |= 1;
  }
  return result;
}

SubsetterImpl::SubsetterImpl() {
}

SubsetterImpl::~SubsetterImpl() {
}

bool SubsetterImpl::LoadFont(const char* font_name,
                             const unsigned char* original_font,
                             size_t font_size) {
  ByteArrayPtr raw_font =
      new MemoryByteArray((byte_t*)original_font, font_size);
  if (factory_ == NULL) {
    factory_.attach(FontFactory::getInstance());
  }

  FontArray font_array;
  factory_->loadFonts(raw_font, &font_array);
  font_ = FindFont(font_name, font_array);
  if (font_ == NULL) {
    return false;
  }

  return true;
}

int SubsetterImpl::SubsetFont(const unsigned int* glyph_ids,
                              size_t glyph_count,
                              unsigned char** output_buffer) {
  if (factory_ == NULL || font_ == NULL) {
    return -1;
  }

  FontPtr new_font;
  new_font.attach(Subset(glyph_ids, glyph_count));
  if (new_font == NULL) {
    return 0;
  }

  MemoryOutputStream output_stream;
  factory_->serializeFont(new_font, &output_stream);
  int length = static_cast<int>(output_stream.size());
  if (length > 0) {
    *output_buffer = new unsigned char[length];
    memcpy(*output_buffer, output_stream.get(), length);
  }

  return length;
}

Font* SubsetterImpl::FindFont(const char* font_name,
                              const FontArray& font_array) {
  if (font_array.empty() || font_array[0] == NULL) {
    return NULL;
  }

  if (font_name && strlen(font_name)) {
    for (FontArray::const_iterator b = font_array.begin(), e = font_array.end();
         b != e; ++b) {
      if (HasName(font_name, (*b).p_)) {
        return (*b).p_;
      }
    }
  }

  return font_array[0].p_;
}

bool SubsetterImpl::HasName(const char* font_name, Font* font) {
  UnicodeString font_string = UnicodeString::fromUTF8(font_name);
  if (font_string.isEmpty())
    return false;
  UnicodeString regular_suffix = UnicodeString::fromUTF8(" Regular");
  UnicodeString alt_font_string = font_string;
  alt_font_string += regular_suffix;

  typedef std::map<int32_t, UnicodeString> NameMap;
  NameMap names;
  NameTablePtr name_table = down_cast<NameTable*>(font->table(Tag::name));

  for (int32_t i = 0; i < name_table->nameCount(); ++i) {
    switch(name_table->nameId(i)) {
      case NameId::kFontFamilyName:
      case NameId::kFontSubfamilyName:
      case NameId::kFullFontName:
      case NameId::kPreferredFamily:
      case NameId::kPreferredSubfamily:
      case NameId::kWWSFamilyName:
      case NameId::kWWSSubfamilyName: {
        int32_t hash_code = HashCode(name_table->platformId(i),
                                      name_table->encodingId(i),
                                      name_table->languageId(i),
                                      name_table->nameId(i));
        UChar* name_part = name_table->name(i);
        ConstructName(name_part, &(names[hash_code]), name_table->nameId(i));
        break;
      }
      default:
        break;
    }
  }

  if (!names.empty()) {
    for (NameMap::iterator b = names.begin(), e = names.end(); b != e; ++b) {
      if (b->second.caseCompare(font_string, 0) == 0 ||
          b->second.caseCompare(alt_font_string, 0) == 0) {
        return true;
      }
    }
  }
  return false;
}

CALLER_ATTACH Font* SubsetterImpl::Subset(const unsigned int* glyph_ids,
                                          size_t glyph_count) {
  if (glyph_ids == NULL || glyph_count == 0) {
    return NULL;
  }

  // Find glyf and loca table.
  GlyphTablePtr glyph_table = down_cast<GlyphTable*>(font_->table(Tag::glyf));
  LocaTablePtr loca_table = down_cast<LocaTable*>(font_->table(Tag::loca));
  if (glyph_table == NULL || loca_table == NULL) {
    // The font is invalid.
    return NULL;
  }

  // Setup font builders we need.
  FontBuilderPtr font_builder;
  font_builder.attach(factory_->newFontBuilder());

  GlyphTableBuilderPtr glyph_table_builder;
  glyph_table_builder.attach(down_cast<GlyphTable::Builder*>(
      font_builder->newTableBuilder(Tag::glyf)));
  LocaTableBuilderPtr loca_table_builder;
  loca_table_builder.attach(down_cast<LocaTable::Builder*>(
      font_builder->newTableBuilder(Tag::loca)));
  if (glyph_table_builder == NULL || loca_table_builder == NULL) {
    // Out of memory.
    return NULL;
  }

  // Sort and uniquify glyph ids.
  IntegerSet glyph_id_set;
  glyph_id_set.insert(0);  // Always include glyph id 0.
  for (size_t i = 0; i < glyph_count; ++i) {
    glyph_id_set.insert(glyph_ids[i]);
  }

  // Extract glyphs and setup loca list.
  IntegerList loca_list;
  loca_list.resize(loca_table->numGlyphs());
  loca_list.push_back(0);
  int32_t last_glyph_id = 0;
  int32_t last_offset = 0;
  GlyphTable::GlyphBuilderList* glyph_builders =
      glyph_table_builder->glyphBuilders();
  for (IntegerSet::iterator i = glyph_id_set.begin(), e = glyph_id_set.end();
       i != e; ++i) {
    if (*i < 0 || *i >= loca_table->numGlyphs()) {
      // Invalid glyph id, ignore.
      continue;
    }

    int32_t length = loca_table->glyphLength(*i);
    if (length == 0) {
      // Empty glyph, ignore.
      continue;
    }
    int32_t offset = loca_table->glyphOffset(*i);

    GlyphPtr glyph;
    glyph.attach(glyph_table->glyph(offset, length));
    if (glyph == NULL) {
      // Error finding glyph, ignore.
      continue;
    }

    // Add glyph to new glyf table.
    ReadableFontDataPtr data = glyph->readFontData();
    WritableFontDataPtr copy_data;
    copy_data.attach(font_builder->getNewData(data->length()));
    data->copyTo(copy_data);
    GlyphBuilderPtr glyph_builder;
    glyph_builder.attach(glyph_table_builder->glyphBuilder(copy_data));
    glyph_builders->push_back(glyph_builder);

    // Configure loca list.
    for (int32_t j = last_glyph_id + 1; j <= *i; ++j) {
      loca_list[j] = last_offset;
    }
    last_offset += length;
    loca_list[*i + 1] = last_offset;
    last_glyph_id = *i;
  }
  for (int32_t j = last_glyph_id + 1; j <= loca_table->numGlyphs(); ++j) {
    loca_list[j] = last_offset;
  }
  loca_table_builder->setLocaList(&loca_list);

  // Setup remaining builders.
  for (TableMap::iterator i = font_->tables()->begin(),
                          e = font_->tables()->end(); i != e; ++i) {
    // We already build the builder for glyph and loca.
    if (i->first != Tag::glyf && i->first != Tag::loca) {
      // The newTableBuilder() call will alter internal state of font_builder
      // AND the reference count of returned object.  Therefore we need to
      // dereference it.
      TableBuilderPtr dereference;
      dereference.attach(
          font_builder->newTableBuilder(i->first, i->second->readFontData()));
    }
  }

  return font_builder->build();
}

}  // namespace sfntly
