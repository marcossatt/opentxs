// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_forward_declare opentxs::protobuf::ContactItemType
// IWYU pragma: no_include <boost/unordered/detail/foa.hpp>
// IWYU pragma: no_include <boost/unordered/detail/foa/table.hpp>

#include "opentxs/identity/wot/claim/Data.hpp"  // IWYU pragma: associated

#include <boost/container/flat_set.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <opentxs/protobuf/ContactData.pb.h>
#include <opentxs/protobuf/ContactItem.pb.h>
#include <opentxs/protobuf/ContactItemType.pb.h>
#include <opentxs/protobuf/ContactSection.pb.h>
#include <opentxs/protobuf/ContactSectionName.pb.h>
#include <algorithm>
#include <functional>
#include <iterator>
#include <span>
#include <sstream>
#include <string_view>
#include <utility>

#include "opentxs/api/Session.hpp"
#include "opentxs/api/session/Crypto.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/identifier/Generic.hpp"
#include "opentxs/identifier/Notary.hpp"
#include "opentxs/identifier/Nym.hpp"
#include "opentxs/identity/wot/Claim.hpp"
#include "opentxs/identity/wot/claim/Attribute.hpp"  // IWYU pragma: keep
#include "opentxs/identity/wot/claim/ClaimType.hpp"  // IWYU pragma: keep
#include "opentxs/identity/wot/claim/Group.hpp"
#include "opentxs/identity/wot/claim/Item.hpp"
#include "opentxs/identity/wot/claim/Section.hpp"
#include "opentxs/identity/wot/claim/SectionType.hpp"  // IWYU pragma: keep
#include "opentxs/identity/wot/claim/Types.hpp"
#include "opentxs/identity/wot/claim/Types.internal.hpp"
#include "opentxs/identity/wot/claim/internal.factory.hpp"
#include "opentxs/protobuf/Types.internal.hpp"
#include "opentxs/protobuf/Types.internal.tpp"
#include "opentxs/protobuf/contact/Types.internal.hpp"
#include "opentxs/protobuf/syntax/VerifyContacts.hpp"
#include "opentxs/util/Log.hpp"
#include "opentxs/util/Writer.hpp"

namespace opentxs::identity::wot::claim
{
static auto extract_sections(
    const api::Session& api,
    const UnallocatedCString& nym,
    const VersionNumber targetVersion,
    const protobuf::ContactData& serialized) -> Data::SectionMap
{
    Data::SectionMap sectionMap{};

    for (const auto& it : serialized.section()) {
        if ((0 != it.version()) && (it.item_size() > 0)) {
            sectionMap[translate(it.name())].reset(new Section(
                api,
                nym,
                check_version(serialized.version(), targetVersion),
                it));
        }
    }

    return sectionMap;
}

struct Data::Imp {
    using Scope =
        std::pair<claim::ClaimType, std::shared_ptr<const claim::Group>>;

    const api::Session& api_;
    const VersionNumber version_{0};
    const UnallocatedCString nym_{};
    const SectionMap sections_{};

    Imp(const api::Session& api,
        const UnallocatedCString& nym,
        const VersionNumber version,
        const VersionNumber targetVersion,
        const SectionMap& sections)
        : api_(api)
        , version_(check_version(version, targetVersion))
        , nym_(nym)
        , sections_(sections)
    {
        if (0 == version) {
            LogError()()("Warning: malformed version. "
                         "Setting to ")(targetVersion)(".")
                .Flush();
        }
    }

    Imp(const Imp& rhs)

        = default;

    auto scope() const -> Scope
    {
        const auto it = sections_.find(claim::SectionType::Scope);

        if (sections_.end() == it) {
            return {claim::ClaimType::Unknown, nullptr};
        }

        assert_false(nullptr == it->second);

        const auto& section = *it->second;

        if (1 != section.Size()) { return {claim::ClaimType::Error, nullptr}; }

        return *section.begin();
    }
};

Data::Data(
    const api::Session& api,
    const UnallocatedCString& nym,
    const VersionNumber version,
    const VersionNumber targetVersion,
    const SectionMap& sections)
    : imp_(std::make_unique<Imp>(api, nym, version, targetVersion, sections))
{
    assert_false(nullptr == imp_);
}

Data::Data(const Data& rhs)
    : imp_(std::make_unique<Imp>(*rhs.imp_))
{
    assert_false(nullptr == imp_);
}

Data::Data(
    const api::Session& api,
    const UnallocatedCString& nym,
    const VersionNumber targetVersion,
    const protobuf::ContactData& serialized)
    : Data(
          api,
          nym,
          serialized.version(),
          targetVersion,
          extract_sections(api, nym, targetVersion, serialized))
{
}

Data::Data(
    const api::Session& api,
    const UnallocatedCString& nym,
    const VersionNumber targetVersion,
    const ReadView& serialized)
    : Data(
          api,
          nym,
          targetVersion,
          protobuf::Factory<protobuf::ContactData>(serialized))
{
}

auto Data::operator+(const Data& rhs) const -> Data
{
    auto map{imp_->sections_};

    for (const auto& it : rhs.imp_->sections_) {
        const auto& rhsID = it.first;
        const auto& rhsSection = it.second;

        assert_false(nullptr == rhsSection);

        auto lhs = map.find(rhsID);
        const bool exists = (map.end() != lhs);

        if (exists) {
            auto& section = lhs->second;

            assert_false(nullptr == section);

            section.reset(new claim::Section(*section + *rhsSection));

            assert_false(nullptr == section);
        } else {
            const auto [i, inserted] = map.emplace(rhsID, rhsSection);

            assert_true(inserted);

            [[maybe_unused]] const auto& notUsed = i;
        }
    }

    const auto version = std::max(imp_->version_, rhs.imp_->version_);

    return {imp_->api_, imp_->nym_, version, version, map};
}

Data::operator UnallocatedCString() const
{
    return PrintContactData([&] {
        auto proto = protobuf::ContactData{};
        Serialize(proto, false);

        return proto;
    }());
}

auto Data::AddContract(
    const UnallocatedCString& instrumentDefinitionID,
    const UnitType currency,
    const bool primary,
    const bool active) const -> Data
{
    bool needPrimary{true};
    const claim::SectionType section{claim::SectionType::Contract};
    auto group = Group(section, UnitToClaim(currency));

    if (group) { needPrimary = group->Primary().empty(); }

    auto attrib = boost::container::flat_set<claim::Attribute>{};

    if (active || primary || needPrimary) {
        attrib.emplace(claim::Attribute::Active);
    }

    if (primary || needPrimary) { attrib.emplace(claim::Attribute::Primary); }

    const auto version = protobuf::RequiredVersion(
        translate(section), translate(UnitToClaim(currency)), imp_->version_);
    auto item = std::make_shared<Item>(factory::ContactItem(
        imp_->api_.Factory().Claim(
            imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
            section,
            UnitToClaim(currency),
            instrumentDefinitionID,
            attrib,
            {},
            {},
            {},
            version,
            {}  // TODO allocator
            ),
        {}  // TODO allocator
        ));

    assert_false(nullptr == item);

    return AddItem(item);
}

auto Data::AddEmail(
    const UnallocatedCString& value,
    const bool primary,
    const bool active) const -> Data
{
    bool needPrimary{true};
    const claim::SectionType section{claim::SectionType::Communication};
    const claim::ClaimType type{claim::ClaimType::Email};
    auto group = Group(section, type);

    if (group) { needPrimary = group->Primary().empty(); }

    auto attrib = boost::container::flat_set<claim::Attribute>{};

    if (active || primary || needPrimary) {
        attrib.emplace(claim::Attribute::Active);
    }

    if (primary || needPrimary) { attrib.emplace(claim::Attribute::Primary); }

    const auto version = protobuf::RequiredVersion(
        translate(section), translate(type), imp_->version_);
    auto item = std::make_shared<Item>(factory::ContactItem(
        imp_->api_.Factory().Claim(
            imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
            section,
            type,
            value,
            attrib,
            {},
            {},
            {},
            version,
            {}  // TODO allocator
            ),
        {}  // TODO allocator
        ));

    assert_false(nullptr == item);

    return AddItem(item);
}

auto Data::AddItem(const wot::Claim& claim) const -> Data
{
    auto item = std::make_shared<Item>(factory::ContactItem(
        claim, {}  // TODO allocator
        ));
    assert_false(nullptr == item);
    item->SetVersion(protobuf::RequiredVersion(
        translate(claim.Section()), translate(claim.Type()), imp_->version_));

    return AddItem(item);
}

auto Data::AddItem(const std::shared_ptr<Item>& item) const -> Data
{
    assert_false(nullptr == item);

    const auto& sectionID = item->Section();
    auto map{imp_->sections_};
    auto it = map.find(sectionID);

    auto version = protobuf::RequiredVersion(
        translate(sectionID), translate(item->Type()), imp_->version_);

    if (map.end() == it) {
        auto& section = map[sectionID];
        section.reset(new claim::Section(
            imp_->api_, imp_->nym_, version, version, sectionID, item));

        assert_false(nullptr == section);
    } else {
        auto& section = it->second;

        assert_false(nullptr == section);

        section.reset(new claim::Section(section->AddItem(item)));

        assert_false(nullptr == section);
    }

    return {imp_->api_, imp_->nym_, version, version, map};
}

auto Data::AddPaymentCode(
    const UnallocatedCString& code,
    const UnitType currency,
    const bool primary,
    const bool active) const -> Data
{
    auto needPrimary{true};
    static constexpr auto section{claim::SectionType::Procedure};
    auto group = Group(section, UnitToClaim(currency));

    if (group) { needPrimary = group->Primary().empty(); }

    const auto attrib = [&] {
        auto out = boost::container::flat_set<claim::Attribute>{};

        if (active || primary || needPrimary) {
            out.emplace(claim::Attribute::Active);
        }

        if (primary || needPrimary) { out.emplace(claim::Attribute::Primary); }

        return out;
    }();
    const auto type = UnitToClaim(currency);
    const auto version = protobuf::RequiredVersion(
        translate(section), translate(type), imp_->version_);

    if (0 == version) {
        LogError()()("This currency is not allowed to set a procedure").Flush();

        return *this;
    }

    auto item = std::make_shared<Item>(factory::ContactItem(
        imp_->api_.Factory().Claim(
            imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
            section,
            type,
            code,
            attrib,
            {},
            {},
            {},
            version,
            {}  // TODO allocator
            ),
        {}  // TODO allocator
        ));

    assert_false(nullptr == item);

    return AddItem(item);
}

auto Data::AddPhoneNumber(
    const UnallocatedCString& value,
    const bool primary,
    const bool active) const -> Data
{
    bool needPrimary{true};
    const claim::SectionType section{claim::SectionType::Communication};
    const claim::ClaimType type{claim::ClaimType::Phone};
    auto group = Group(section, type);

    if (group) { needPrimary = group->Primary().empty(); }

    auto attrib = boost::container::flat_set<claim::Attribute>{};

    if (active || primary || needPrimary) {
        attrib.emplace(claim::Attribute::Active);
    }

    if (primary || needPrimary) { attrib.emplace(claim::Attribute::Primary); }

    const auto version = protobuf::RequiredVersion(
        translate(section), translate(type), imp_->version_);
    auto item = std::make_shared<Item>(factory::ContactItem(
        imp_->api_.Factory().Claim(
            imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
            section,
            type,
            value,
            attrib,
            {},
            {},
            {},
            version,
            {}  // TODO allocator
            ),
        {}  // TODO allocator
        ));

    assert_false(nullptr == item);

    return AddItem(item);
}

auto Data::AddPreferredOTServer(
    const identifier::Generic& id,
    const bool primary) const -> Data
{
    bool needPrimary{true};
    const claim::SectionType section{claim::SectionType::Communication};
    const claim::ClaimType type{claim::ClaimType::Opentxs};
    auto group = Group(section, type);

    if (group) { needPrimary = group->Primary().empty(); }

    auto attrib = boost::container::flat_set{claim::Attribute::Active};

    if (primary || needPrimary) { attrib.emplace(claim::Attribute::Primary); }

    const auto version = protobuf::RequiredVersion(
        translate(section), translate(type), imp_->version_);
    auto item = std::make_shared<Item>(factory::ContactItem(
        imp_->api_.Factory().Claim(
            imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
            section,
            type,
            id.asBase58(imp_->api_.Crypto()),
            attrib,
            {},
            {},
            {},
            version,
            {}  // TODO allocator
            ),
        {}  // TODO allocator
        ));

    assert_false(nullptr == item);

    return AddItem(item);
}

auto Data::AddSocialMediaProfile(
    const UnallocatedCString& value,
    const claim::ClaimType type,
    const bool primary,
    const bool active) const -> Data
{
    auto map = imp_->sections_;
    // Add the item to the profile section.
    auto& section = map[claim::SectionType::Profile];

    bool needPrimary{true};
    if (section) {
        auto group = section->Group(type);

        if (group) { needPrimary = group->Primary().empty(); }
    }

    auto attrib = boost::container::flat_set<claim::Attribute>{};

    if (active || primary || needPrimary) {
        attrib.emplace(claim::Attribute::Active);
    }

    if (primary || needPrimary) { attrib.emplace(claim::Attribute::Primary); }

    const auto version = protobuf::RequiredVersion(
        translate(claim::SectionType::Profile),
        translate(type),
        imp_->version_);
    auto item = std::make_shared<Item>(factory::ContactItem(
        imp_->api_.Factory().Claim(
            imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
            claim::SectionType::Profile,
            type,
            value,
            attrib,
            {},
            {},
            {},
            version,
            {}  // TODO allocator
            ),
        {}  // TODO allocator
        ));

    assert_false(nullptr == item);

    if (section) {
        section.reset(new claim::Section(section->AddItem(item)));
    } else {
        section.reset(new claim::Section(
            imp_->api_,
            imp_->nym_,
            version,
            version,
            claim::SectionType::Profile,
            item));
    }

    assert_false(nullptr == section);

    // Add the item to the communication section.
    auto commSectionTypes = protobuf::contact::AllowedItemTypes().at(
        protobuf::contact::ContactSectionVersion(
            version, translate(claim::SectionType::Communication)));
    if (commSectionTypes.count(translate(type))) {
        auto& commSection = map[claim::SectionType::Communication];

        if (commSection) {
            auto group = commSection->Group(type);

            if (group) { needPrimary = group->Primary().empty(); }
        }

        attrib.clear();

        if (active || primary || needPrimary) {
            attrib.emplace(claim::Attribute::Active);
        }

        if (primary || needPrimary) {
            attrib.emplace(claim::Attribute::Primary);
        }

        item = std::make_shared<Item>(factory::ContactItem(
            imp_->api_.Factory().Claim(
                imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
                claim::SectionType::Communication,
                type,
                value,
                attrib,
                {},
                {},
                {},
                version,
                {}  // TODO allocator
                ),
            {}  // TODO allocator
            ));

        assert_false(nullptr == item);

        if (commSection) {
            commSection.reset(new claim::Section(commSection->AddItem(item)));
        } else {
            commSection.reset(new claim::Section(
                imp_->api_,
                imp_->nym_,
                version,
                version,
                claim::SectionType::Communication,
                item));
        }

        assert_false(nullptr == commSection);
    }

    // Add the item to the identifier section.
    auto identifierSectionTypes = protobuf::contact::AllowedItemTypes().at(
        protobuf::contact::ContactSectionVersion(
            version, translate(claim::SectionType::Identifier)));
    if (identifierSectionTypes.count(translate(type))) {
        auto& identifierSection = map[claim::SectionType::Identifier];

        if (identifierSection) {
            auto group = identifierSection->Group(type);

            if (group) { needPrimary = group->Primary().empty(); }
        }

        attrib.clear();

        if (active || primary || needPrimary) {
            attrib.emplace(claim::Attribute::Active);
        }

        if (primary || needPrimary) {
            attrib.emplace(claim::Attribute::Primary);
        }

        item = std::make_shared<Item>(factory::ContactItem(
            imp_->api_.Factory().Claim(
                imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
                claim::SectionType::Identifier,
                type,
                value,
                attrib,
                {},
                {},
                {},
                version,
                {}  // TODO allocator
                ),
            {}  // TODO allocator
            ));

        assert_false(nullptr == item);

        if (identifierSection) {
            identifierSection.reset(
                new claim::Section(identifierSection->AddItem(item)));
        } else {
            identifierSection.reset(new claim::Section(
                imp_->api_,
                imp_->nym_,
                version,
                version,
                claim::SectionType::Identifier,
                item));
        }

        assert_false(nullptr == identifierSection);
    }

    return {imp_->api_, imp_->nym_, version, version, map};
}

auto Data::begin() const -> Data::SectionMap::const_iterator
{
    return imp_->sections_.begin();
}

auto Data::BestEmail() const -> UnallocatedCString
{
    UnallocatedCString bestEmail;

    auto group =
        Group(claim::SectionType::Communication, claim::ClaimType::Email);

    if (group) {
        const std::shared_ptr<Item> best = group->Best();

        if (best) { bestEmail = best->Value(); }
    }

    return bestEmail;
}

auto Data::BestPhoneNumber() const -> UnallocatedCString
{
    UnallocatedCString bestEmail;

    auto group =
        Group(claim::SectionType::Communication, claim::ClaimType::Phone);

    if (group) {
        const std::shared_ptr<Item> best = group->Best();

        if (best) { bestEmail = best->Value(); }
    }

    return bestEmail;
}

auto Data::BestSocialMediaProfile(const claim::ClaimType type) const
    -> UnallocatedCString
{
    UnallocatedCString bestProfile;

    auto group = Group(claim::SectionType::Profile, type);
    if (group) {
        const std::shared_ptr<Item> best = group->Best();

        if (best) { bestProfile = best->Value(); }
    }

    return bestProfile;
}

auto Data::Claim(const identifier::Generic& item) const -> std::shared_ptr<Item>
{
    for (const auto& it : imp_->sections_) {
        const auto& section = it.second;

        assert_false(nullptr == section);

        auto claim = section->Claim(item);

        if (claim) { return claim; }
    }

    return {};
}

auto Data::Contracts(const UnitType currency, const bool onlyActive) const
    -> UnallocatedSet<identifier::Generic>
{
    UnallocatedSet<identifier::Generic> output{};
    const claim::SectionType section{claim::SectionType::Contract};
    auto group = Group(section, UnitToClaim(currency));

    if (group) {
        for (const auto& it : *group) {
            const auto& id = it.first;

            assert_false(nullptr == it.second);

            const auto& claim = *it.second;

            if ((false == onlyActive) ||
                claim.HasAttribute(Attribute::Active)) {
                output.insert(id);
            }
        }
    }

    return output;
}

auto Data::Delete(const identifier::Generic& id) const -> Data
{
    bool deleted{false};
    auto map = imp_->sections_;

    for (auto& it : map) {
        auto& section = it.second;

        assert_false(nullptr == section);

        if (section->HaveClaim(id)) {
            section.reset(new claim::Section(section->Delete(id)));

            assert_false(nullptr == section);

            deleted = true;

            if (0 == section->Size()) { map.erase(it.first); }

            break;
        }
    }

    if (false == deleted) { return *this; }

    return {imp_->api_, imp_->nym_, imp_->version_, imp_->version_, map};
}

auto Data::EmailAddresses(bool active) const -> UnallocatedCString
{
    std::ostringstream stream;

    auto group =
        Group(claim::SectionType::Communication, claim::ClaimType::Email);
    if (group) {
        for (const auto& it : *group) {
            assert_false(nullptr == it.second);

            const auto& claim = *it.second;

            if ((false == active) || claim.HasAttribute(Attribute::Active)) {
                stream << claim.Value() << ',';
            }
        }
    }

    UnallocatedCString output = stream.str();

    if (0 < output.size()) { output.erase(output.size() - 1, 1); }

    return output;
}

auto Data::end() const -> Data::SectionMap::const_iterator
{
    return imp_->sections_.end();
}

auto Data::Group(const claim::SectionType section, const claim::ClaimType type)
    const -> std::shared_ptr<claim::Group>
{
    const auto it = imp_->sections_.find(section);

    if (imp_->sections_.end() == it) { return {}; }

    assert_false(nullptr == it->second);

    return it->second->Group(type);
}

auto Data::HaveClaim(const identifier::Generic& item) const -> bool
{
    for (const auto& section : imp_->sections_) {
        assert_false(nullptr == section.second);

        if (section.second->HaveClaim(item)) { return true; }
    }

    return false;
}

auto Data::HaveClaim(
    const claim::SectionType section,
    const claim::ClaimType type,
    std::string_view value) const -> bool
{
    auto group = Group(section, type);

    if (false == bool(group)) { return false; }

    for (const auto& it : *group) {
        assert_false(nullptr == it.second);

        const auto& claim = *it.second;

        if (value == claim.Value()) { return true; }
    }

    return false;
}

auto Data::Name() const -> UnallocatedCString
{
    auto group = imp_->scope().second;

    if (false == bool(group)) { return {}; }

    auto claim = group->Best();

    if (false == bool(claim)) { return {}; }

    return UnallocatedCString{claim->Value()};
}

auto Data::PhoneNumbers(bool active) const -> UnallocatedCString
{
    std::ostringstream stream;

    auto group =
        Group(claim::SectionType::Communication, claim::ClaimType::Phone);
    if (group) {
        for (const auto& it : *group) {
            assert_false(nullptr == it.second);

            const auto& claim = *it.second;

            if ((false == active) || claim.HasAttribute(Attribute::Active)) {
                stream << claim.Value() << ',';
            }
        }
    }

    UnallocatedCString output = stream.str();

    if (0 < output.size()) { output.erase(output.size() - 1, 1); }

    return output;
}

auto Data::PreferredOTServer() const -> identifier::Notary
{
    auto group =
        Group(claim::SectionType::Communication, claim::ClaimType::Opentxs);

    if (false == bool(group)) { return {}; }

    auto claim = group->Best();

    if (false == bool(claim)) { return {}; }

    return imp_->api_.Factory().NotaryIDFromBase58(claim->Value());
}

auto Data::PrintContactData(const protobuf::ContactData& data)
    -> UnallocatedCString
{
    std::stringstream output;
    output << "Version " << data.version() << " contact data" << std::endl;
    output << "Sections found: " << data.section().size() << std::endl;

    for (const auto& section : data.section()) {
        output << "- Section: "
               << protobuf::TranslateSectionName(section.name())
               << ", version: " << section.version() << " containing "
               << section.item().size() << " item(s)." << std::endl;

        for (const auto& item : section.item()) {
            output << "-- Item type: \""
                   << protobuf::TranslateItemType(item.type())
                   << "\", value: \"" << item.value()
                   << "\", start: " << item.start() << ", end: " << item.end()
                   << ", version: " << item.version() << std::endl
                   << "--- Attributes: ";

            for (const auto& attribute : item.attribute()) {
                output << protobuf::TranslateItemAttributes(translate(
                              static_cast<claim::Attribute>(attribute)))
                       << " ";
            }

            output << std::endl;
        }
    }

    return output.str();
}

auto Data::Section(const claim::SectionType section) const
    -> std::shared_ptr<claim::Section>
{
    const auto it = imp_->sections_.find(section);

    if (imp_->sections_.end() == it) { return {}; }

    return it->second;
}

auto Data::SetCommonName(const UnallocatedCString& name) const -> Data
{
    const claim::SectionType section{claim::SectionType::Identifier};
    const claim::ClaimType type{claim::ClaimType::Commonname};
    auto attrib = boost::container::flat_set{
        claim::Attribute::Active, claim::Attribute::Primary};
    const auto version = protobuf::RequiredVersion(
        translate(section), translate(type), imp_->version_);
    auto item = std::make_shared<Item>(factory::ContactItem(
        imp_->api_.Factory().Claim(
            imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
            section,
            type,
            name,
            attrib,
            {},
            {},
            {},
            version,
            {}  // TODO allocator
            ),
        {}  // TODO allocator
        ));

    assert_false(nullptr == item);

    return AddItem(item);
}

auto Data::SetName(const UnallocatedCString& name, const bool primary) const
    -> Data
{
    const Imp::Scope& scopeInfo = imp_->scope();

    assert_false(nullptr == scopeInfo.second);

    const claim::SectionType section{claim::SectionType::Scope};
    const claim::ClaimType type = scopeInfo.first;

    auto attrib = boost::container::flat_set{claim::Attribute::Active};

    if (primary) { attrib.emplace(claim::Attribute::Primary); }

    const auto version = protobuf::RequiredVersion(
        translate(section), translate(type), imp_->version_);
    auto item = std::make_shared<Item>(factory::ContactItem(
        imp_->api_.Factory().Claim(
            imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
            section,
            type,
            name,
            attrib,
            {},
            {},
            {},
            version,
            {}  // TODO allocator
            ),
        {}  // TODO allocator
        ));

    assert_false(nullptr == item);

    return AddItem(item);
}

auto Data::SetScope(const claim::ClaimType type, const UnallocatedCString& name)
    const -> Data
{
    assert_true(claim::ClaimType::Error != type);

    const claim::SectionType section{claim::SectionType::Scope};

    if (claim::ClaimType::Unknown == imp_->scope().first) {
        auto mapCopy = imp_->sections_;
        mapCopy.erase(section);
        auto attrib = boost::container::flat_set{
            claim::Attribute::Active, claim::Attribute::Primary};
        const auto version = protobuf::RequiredVersion(
            translate(section), translate(type), imp_->version_);
        auto item = std::make_shared<Item>(factory::ContactItem(
            imp_->api_.Factory().Claim(
                imp_->api_.Factory().NymIDFromBase58(imp_->nym_),
                section,
                type,
                name,
                attrib,
                {},
                {},
                {},
                version,
                {}  // TODO allocator
                ),
            {}  // TODO allocator
            ));

        assert_false(nullptr == item);

        auto newSection = std::make_shared<claim::Section>(
            imp_->api_, imp_->nym_, version, version, section, item);

        assert_false(nullptr == newSection);

        mapCopy[section] = newSection;

        return {imp_->api_, imp_->nym_, version, version, mapCopy};
    } else {
        LogError()()("Scope already set.").Flush();

        return *this;
    }
}

auto Data::Serialize(Writer&& destination, const bool withID) const -> bool
{
    return write(
        [&] {
            auto proto = protobuf::ContactData{};
            Serialize(proto);

            return proto;
        }(),
        std::move(destination));
}

auto Data::Serialize(protobuf::ContactData& output, const bool withID) const
    -> bool
{
    output.set_version(imp_->version_);

    for (const auto& it : imp_->sections_) {
        const auto& section = it.second;

        assert_false(nullptr == section);

        section->SerializeTo(output, withID);
    }

    return true;
}

auto Data::SocialMediaProfiles(const claim::ClaimType type, bool active) const
    -> UnallocatedCString
{
    std::ostringstream stream;

    auto group = Group(claim::SectionType::Profile, type);
    if (group) {
        for (const auto& it : *group) {
            assert_false(nullptr == it.second);

            const auto& claim = *it.second;

            if ((false == active) || claim.HasAttribute(Attribute::Active)) {
                stream << claim.Value() << ',';
            }
        }
    }

    UnallocatedCString output = stream.str();

    if (0 < output.size()) { output.erase(output.size() - 1, 1); }

    return output;
}

auto Data::SocialMediaProfileTypes() const
    -> const UnallocatedSet<claim::ClaimType>
{
    try {
        auto profiletypes = protobuf::contact::AllowedItemTypes().at(
            protobuf::contact::ContactSectionVersion(
                DefaultVersion(), protobuf::CONTACTSECTION_PROFILE));

        UnallocatedSet<claim::ClaimType> output;
        std::ranges::transform(
            profiletypes,
            std::inserter(output, output.end()),
            [](protobuf::ContactItemType itemtype) -> claim::ClaimType {
                return translate(itemtype);
            });

        return output;

    } catch (...) {
        return {};
    }
}

auto Data::Type() const -> claim::ClaimType { return imp_->scope().first; }

auto Data::Version() const -> VersionNumber { return imp_->version_; }

Data::~Data() = default;
}  // namespace opentxs::identity::wot::claim
