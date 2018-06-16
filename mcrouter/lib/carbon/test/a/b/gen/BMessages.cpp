/*
 *  Copyright (c) 2017-present, Facebook, Inc.
 *
 *  This source code is licensed under the MIT license found in the LICENSE
 *  file in the root directory of this source tree.
 *
 */

/*
 *  THIS FILE IS AUTOGENERATED. DO NOT MODIFY IT; ALL CHANGES WILL BE LOST IN
 *  VAIN.
 *
 *  @generated
 */
#include "BMessages.h"

namespace carbon {
namespace test2 {
namespace util {

std::string enumSimpleEnumToString(SimpleEnum val) {
  switch (val) {
    case SimpleEnum::Twenty:
      return "Twenty";
    case SimpleEnum::Zero:
      return "Zero";
    case SimpleEnum::One:
      return "One";
    case SimpleEnum::Negative:
      return "Negative";
  }
  return "<INVALID_OPTION>";
}

void SimpleStruct::serialize(carbon::CarbonProtocolWriter& writer) const {
  writer.writeStructBegin();
  writer.writeField(1 /* field id */, member1());
  writer.writeFieldStop();
  writer.writeStructEnd();
}

void SimpleStruct::deserialize(carbon::CarbonProtocolReader& reader) {
  reader.readStructBegin();
  while (true) {
    const auto pr = reader.readFieldHeader();
    const auto fieldType = pr.first;
    const auto fieldId = pr.second;

    if (fieldType == carbon::FieldType::Stop) {
      break;
    }

    switch (fieldId) {
      case 1: {
        reader.readField(member1(), fieldType);
        break;
      }
      default: {
        reader.skip(fieldType);
        break;
      }
    }
  }
  reader.readStructEnd();
}

void SimpleUnion::serialize(carbon::CarbonProtocolWriter& writer) const {
  writer.writeStructBegin();
  switch (_which_) {
    case 1: {
      writer.writeFieldAlways(1 /* field id */, umember1());
      break;
    }
    case 2: {
      writer.writeFieldAlways(2 /* field id */, umember2());
      break;
    }
    case 3: {
      writer.writeFieldAlways(3 /* field id */, umember3());
      break;
    }
    default:
      break;
  }
  writer.writeFieldStop();
  writer.writeStructEnd();
}

void SimpleUnion::deserialize(carbon::CarbonProtocolReader& reader) {
  reader.readStructBegin();
  while (true) {
    const auto pr = reader.readFieldHeader();
    const auto fieldType = pr.first;
    const auto fieldId = pr.second;

    if (fieldType == carbon::FieldType::Stop) {
      break;
    }

    switch (fieldId) {
      case 1: {
        reader.readRawInto(emplace<1>());
        break;
      }
      case 2: {
        reader.readRawInto(emplace<2>());
        break;
      }
      case 3: {
        reader.readRawInto(emplace<3>());
        break;
      }
      default: {
        reader.skip(fieldType);
        break;
      }
    }
  }
  reader.readStructEnd();
}

constexpr const char* const YetAnotherRequest::name;

void YetAnotherRequest::serialize(carbon::CarbonProtocolWriter& writer) const {
  writer.writeStructBegin();
  writer.writeField(1 /* field id */, key());
  writer.writeFieldStop();
  writer.writeStructEnd();
}

void YetAnotherRequest::deserialize(carbon::CarbonProtocolReader& reader) {
  reader.readStructBegin();
  while (true) {
    const auto pr = reader.readFieldHeader();
    const auto fieldType = pr.first;
    const auto fieldId = pr.second;

    if (fieldType == carbon::FieldType::Stop) {
      break;
    }

    switch (fieldId) {
      case 1: {
        reader.readField(key(), fieldType);
        break;
      }
      default: {
        reader.skip(fieldType);
        break;
      }
    }
  }
  reader.readStructEnd();
}

void YetAnotherReply::serialize(carbon::CarbonProtocolWriter& writer) const {
  writer.writeStructBegin();
  writer.writeField(1 /* field id */, result());
  writer.writeFieldStop();
  writer.writeStructEnd();
}

void YetAnotherReply::deserialize(carbon::CarbonProtocolReader& reader) {
  reader.readStructBegin();
  while (true) {
    const auto pr = reader.readFieldHeader();
    const auto fieldType = pr.first;
    const auto fieldId = pr.second;

    if (fieldType == carbon::FieldType::Stop) {
      break;
    }

    switch (fieldId) {
      case 1: {
        reader.readField(result(), fieldType);
        break;
      }
      default: {
        reader.skip(fieldType);
        break;
      }
    }
  }
  reader.readStructEnd();
}
} // namespace util
} // namespace test2
} // namespace carbon
