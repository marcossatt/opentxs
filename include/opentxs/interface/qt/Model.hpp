// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <QAbstractItemModel>
#include <QByteArray>
#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QVariant>
#include <memory>

#include "opentxs/Export.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace ui
{
namespace internal
{
struct Row;
}  // namespace internal

namespace qt
{
namespace internal
{
struct Index;
struct Model;
}  // namespace internal

class Model;
}  // namespace qt
}  // namespace ui
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::ui::qt
{
class OPENTXS_EXPORT RowWrapper final : public QObject
{
    Q_OBJECT
public:
    std::shared_ptr<opentxs::ui::internal::Row> row_;

    RowWrapper() noexcept;
    OPENTXS_NO_EXPORT RowWrapper(
        std::shared_ptr<opentxs::ui::internal::Row>) noexcept;
    RowWrapper(const RowWrapper&) noexcept;
    RowWrapper(RowWrapper&&) = delete;
    auto operator=(const RowWrapper&) noexcept -> RowWrapper&;
    auto operator=(RowWrapper&&) noexcept -> RowWrapper& = delete;

    ~RowWrapper() final;

private:
};
}  // namespace opentxs::ui::qt

Q_DECLARE_OPAQUE_POINTER(opentxs::ui::internal::Row*)
Q_DECLARE_METATYPE(opentxs::ui::qt::RowWrapper)
Q_DECLARE_METATYPE(opentxs::ui::internal::Row*)

namespace opentxs::ui::qt
{
class ModelHelper final : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void changeRow(ui::internal::Row* parent, ui::internal::Row* row);
    void deleteRow(ui::internal::Row* row);
    void insertRow(
        ui::internal::Row* parent,
        ui::internal::Row* after,
        opentxs::ui::qt::RowWrapper row);
    void moveRow(
        ui::internal::Row* newParent,
        ui::internal::Row* newBefore,
        ui::internal::Row* row);
    void startupComplete();

public Q_SLOTS:
    void requestChangeRow(
        ui::internal::Row* parent,
        ui::internal::Row* row) noexcept;
    void requestDeleteRow(ui::internal::Row* row) noexcept;
    void requestInsertRow(
        ui::internal::Row* parent,
        ui::internal::Row* after,
        std::shared_ptr<ui::internal::Row> row) noexcept;
    void requestMoveRow(
        ui::internal::Row* newParent,
        ui::internal::Row* newBefore,
        ui::internal::Row* row) noexcept;
    void setStartupComplete() noexcept;

public:
    OPENTXS_NO_EXPORT ModelHelper(Model* model) noexcept;
    ModelHelper() = delete;
    ModelHelper(const ModelHelper&) = delete;
    ModelHelper(ModelHelper&&) = delete;
    auto operator=(const ModelHelper&) -> ModelHelper& = delete;
    auto operator=(ModelHelper&&) -> ModelHelper& = delete;

    ~ModelHelper() final;
};

class OPENTXS_EXPORT Model : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(
        bool startupComplete READ startupIsComplete NOTIFY startupComplete)

Q_SIGNALS:
    void startupComplete() const;

public:
    auto columnCount(const QModelIndex& parent = {}) const noexcept
        -> int final;
    auto data(const QModelIndex& index, int role = Qt::DisplayRole)
        const noexcept -> QVariant final;
    auto hasChildren(const QModelIndex& parent = {}) const noexcept
        -> bool final;
    auto headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole) const noexcept -> QVariant override;
    auto index(int row, int column, const QModelIndex& parent = {})
        const noexcept -> QModelIndex final;
    auto parent(const QModelIndex& index) const noexcept -> QModelIndex final;
    auto roleNames() const noexcept -> QHash<int, QByteArray> final;
    auto rowCount(const QModelIndex& parent = {}) const noexcept -> int final;
    auto startupIsComplete() const noexcept -> bool;

    Model() = delete;
    Model(const Model&) = delete;
    Model(Model&&) = delete;
    auto operator=(const Model&) -> Model& = delete;
    auto operator=(Model&&) -> Model& = delete;

    ~Model() override;

protected:
    internal::Model* internal_;

    Model(internal::Model* internal) noexcept;

private Q_SLOTS:
    void changeRow(ui::internal::Row* parent, ui::internal::Row* row) noexcept;
    void deleteRow(ui::internal::Row* row) noexcept;
    void insertRow(
        ui::internal::Row* parent,
        ui::internal::Row* after,
        opentxs::ui::qt::RowWrapper row) noexcept;
    void moveRow(
        ui::internal::Row* newParent,
        ui::internal::Row* newBefore,
        ui::internal::Row* row) noexcept;
    void setStartupComplete() noexcept;

private:
    friend ModelHelper;

    auto make_index(const internal::Index& index) const noexcept -> QModelIndex;
};
}  // namespace opentxs::ui::qt
