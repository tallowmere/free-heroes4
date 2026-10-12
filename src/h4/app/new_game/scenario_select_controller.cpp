#include "h4/app/new_game/scenario_select_controller.hpp"

#include <array>
#include <format>
#include <string>
#include <utility>

#include "h4/assets/assets.hpp"

namespace h4::app {

namespace {

constexpr std::array<std::string_view, 3> kMapTypeIcons{"original", "expansion", "expansion_2"};

constexpr std::string_view size_letter(model::MapSize size) noexcept {
    switch (size) {
    case model::MapSize::small: return "S";
    case model::MapSize::medium: return "M";
    case model::MapSize::large: return "L";
    case model::MapSize::extra_large: return "XL";
    }
    return "";
}

constexpr model::ScenarioOrder order_for(screens::ScenarioColumn column) noexcept {
    switch (column) {
    case screens::ScenarioColumn::name: return model::ScenarioOrder::name;
    case screens::ScenarioColumn::size: return model::ScenarioOrder::size;
    case screens::ScenarioColumn::difficulty: return model::ScenarioOrder::difficulty;
    case screens::ScenarioColumn::players: return model::ScenarioOrder::players;
    case screens::ScenarioColumn::humans: return model::ScenarioOrder::humans;
    case screens::ScenarioColumn::allies: return model::ScenarioOrder::allies;
    case screens::ScenarioColumn::map_type: return model::ScenarioOrder::map_type;
    }
    return model::ScenarioOrder::name;
}

}

ScenarioSelectController::ScenarioSelectController(screens::ScenarioSelectView& view,
                                                   screens::PopupHost& dialogs,
                                                   const assets::Assets& assets,
                                                   Reporter report)
: view_{&view}, details_{dialogs}, options_{dialogs}, assets_{&assets}, report_{std::move(report)} {
    connections_.on(view_->button_clicked, [this](screens::ScenarioButton button) { on_button(button); });
    connections_.on(view_->row_selected, [this](std::size_t row) { on_selected(row); });
    connections_.on(view_->row_activated, [this](std::size_t row) { on_activated(row); });
    connections_.on(view_->sort_requested, [this](screens::ScenarioColumn column) { on_sort(column); });
}

void ScenarioSelectController::on_button(screens::ScenarioButton button) {
    switch (button) {
    case screens::ScenarioButton::cancel: closed.emit(); break;
    case screens::ScenarioButton::details: open_details(); break;
    case screens::ScenarioButton::next: open_options(); break;
    }
}

core::Result<void> ScenarioSelectController::open_folder(const std::filesystem::path& dir) {
    auto scanned = model::ScenarioFolder::scan(dir);
    if (!scanned)
        return std::unexpected{scanned.error()};
    list(std::move(*scanned));
    return {};
}

void ScenarioSelectController::list(model::ScenarioFolder scanned) {
    folder_ = scanned.sorted(order_, direction_);
    show(folder_->first_scenario().value_or(0));
}

void ScenarioSelectController::on_frame() {
    auto scanned = scan_.take();
    if (!scanned || view_ == nullptr)
        return;
    if (report_.failed(*scanned))
        return;
    list(std::move(**scanned));
}

void ScenarioSelectController::show(std::size_t selected) {
    selected_ = selected;
    update_description();
    view_->bind(make_model());
}

void ScenarioSelectController::update_description() {
    description_.clear();
    next_enabled_ = false;
    if (selected_ < folder_->entry_count() && folder_->entry(selected_).kind == model::FolderEntryKind::scenario) {
        description_  = folder_->entry(selected_).scenario->description;
        next_enabled_ = true;
    }
}

screens::ScenarioSelectModel ScenarioSelectController::make_model() const {
    screens::ScenarioSelectModel model;
    model.columns      = columns_;
    model.selected     = selected_;
    model.description  = description_;
    model.next_enabled = next_enabled_;
    model.rows.reserve(folder_->entry_count());
    for (std::size_t i = 0; i < folder_->entry_count(); ++i)
        model.rows.push_back(row_for(folder_->entry(i)));
    return model;
}

screens::ScenarioRow ScenarioSelectController::row_for(const model::FolderEntry& entry) const {
    screens::ScenarioRow row;
    switch (entry.kind) {
    case model::FolderEntryKind::parent: row.name = std::format("[{}]", assets_->text("parent_folder")); break;
    case model::FolderEntryKind::subfolder: row.name = std::format("[{}]", entry.name); break;
    case model::FolderEntryKind::scenario: {
        const model::ScenarioSummary& s = *entry.scenario;
        row.name                        = s.name;
        row.size                        = size_letter(s.size);
        row.players                     = std::to_string(s.players);
        row.humans                      = std::to_string(s.humans);
        row.allies                      = std::to_string(s.alliances);
        row.difficulty_icon             = model::to_string(s.difficulty);
        if (s.edition)
            row.map_type_icon = std::format("{}{}", kMapTypeIcons[std::to_underlying(*s.edition)], s.multiplayer ? "_multiplayer" : "");
        break;
    }
    }
    return row;
}

void ScenarioSelectController::on_selected(std::size_t entry) {
    selected_ = entry;
    update_description();
    view_->bind(make_model());
}

void ScenarioSelectController::on_activated(std::size_t entry) {
    if (!folder_ || entry >= folder_->entry_count())
        return;
    const model::FolderEntry target = folder_->entry(entry);
    if (target.kind == model::FolderEntryKind::scenario) {
        open_options();
        return;
    }
    scan_ = core::Pending<core::Result<model::ScenarioFolder>>::run([dir = target.target](std::stop_token stop) {
        return model::ScenarioFolder::scan(dir, stop);
    });
}

void ScenarioSelectController::on_sort(screens::ScenarioColumn column) {
    if (!folder_ || selected_ >= folder_->entry_count())
        return;
    const model::FolderEntry selected = folder_->entry(selected_);
    const model::ScenarioOrder order  = order_for(column);
    if (order == order_)
        direction_ = direction_ == model::SortDirection::ascending ? model::SortDirection::descending :
                                                                     model::SortDirection::ascending;
    else
        direction_ = model::SortDirection::ascending;
    order_  = order;
    folder_ = folder_->sorted(order_, direction_);
    const std::size_t index = selected.kind == model::FolderEntryKind::scenario ? folder_->find(selected.target).value_or(0) : selected_;
    show(index);
}

void ScenarioSelectController::open_options() {
    if (!folder_ || selected_ >= folder_->entry_count() || !next_enabled_)
        return;
    const model::FolderEntry entry = folder_->entry(selected_);
    if (entry.scenario == nullptr)
        return;
    auto options = screens::ScenarioOptionsView::build(*assets_, view_->frame_size(), screens::ScenarioOptionsModel{});
    if (report_.failed(options))
        return;
    auto controller = std::make_unique<ScenarioOptionsController>(**options, *assets_, *entry.scenario, guards_move_, report_);
    options_.open(std::move(*options),
                  std::move(controller),
                  [this, file = entry.scenario->path](screens::ScenarioOptionsView&, ScenarioOptionsController& opened, core::Connections& wiring) {
                      wiring.on(opened.closed, [this] { close_options(); });
                      wiring.on(opened.began, [this, file](model::ScenarioSetup setup) {
                          close_options();
                          began.emit(file, std::move(setup));
                      });
                  },
                  {[this] { close_options(); }, [this] { options_.controller()->begin(); }});
}

void ScenarioSelectController::close_options() {
    options_.close();
}

void ScenarioSelectController::detach() noexcept {
    view_ = nullptr;
    details_.forget();
    options_.forget();
    scan_ = {};
}

bool ScenarioSelectController::on_key(platform::Key key) {
    return options_.on_key(key) || details_.on_key(key);
}

void ScenarioSelectController::open_details() {
    auto details = screens::ScenarioDetailsView::build(*assets_, view_->frame_size(), screens::ScenarioDetailsModel{columns_});
    if (report_.failed(details))
        return;
    details_.open(std::move(*details),
                  [this](screens::ScenarioDetailsView& opened, core::Connections& wiring) {
                      wiring.on(opened.cancelled, [this] { close_details(); });
                      wiring.on(opened.accepted, [this](screens::ScenarioColumnSet columns) {
                          columns_ = columns;
                          view_->bind(make_model());
                          close_details();
                      });
                  },
                  {[this] { close_details(); }, {}});
}

void ScenarioSelectController::close_details() {
    details_.close();
}

}
