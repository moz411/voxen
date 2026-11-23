#pragma once
#include <cstddef>
#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace voxen::ui {
struct SurfaceMedia {
    int kind=0; // 0 local Sobel override, 1 picture, 2 video, 3 shader
    std::string filename;
    int shaderMode=0;
    bool operator==(const SurfaceMedia&) const = default;
};

// The sole mutable source of truth is keyed by persistent spatial UUID.
// Every explicit operation is reversible; preview/selection never write here.
class SurfaceMediaEditor {
public:
    using Assignments=std::unordered_map<std::string,SurfaceMedia>;
    struct Change {
        std::string uuid;
        std::optional<SurfaceMedia> before,after;
    };
    [[nodiscard]] const Assignments& assignments() const noexcept {return assignments_;}
    [[nodiscard]] const SurfaceMedia* find(const std::string& uuid) const {
        auto it=assignments_.find(uuid);
        return it==assignments_.end()?nullptr:&it->second;
    }
    bool assign(const std::string& uuid,std::optional<SurfaceMedia> value) {
        if(uuid.empty() || uuid.size()>80 ||
           !std::all_of(uuid.begin(),uuid.end(),[](char c){return
               (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')||c=='-';}) ||
           (value && assignments_.size()>=256 && !find(uuid)) || (value &&
           (value->kind<0 || value->kind>3 ||
            value->shaderMode<0 || value->shaderMode>255 ||
            value->filename.size()>128 || value->filename.find('/')!=std::string::npos ||
            value->filename.find('\\')!=std::string::npos ||
            value->filename.find("..")!=std::string::npos ||
            value->filename.find_first_of(" \t\r\n")!=std::string::npos ||
            (value->kind!=3 && value->filename.empty()))))return false;
        std::optional<SurfaceMedia> before;
        if(const auto* previous=find(uuid))before=*previous;
        if(before==value)return false;
        if(cursor_<history_.size())history_.resize(cursor_);
        history_.push_back({uuid,before,value});
        ++cursor_;
        apply(uuid,value);
        return true;
    }
    bool undo() {
        if(cursor_==0)return false;
        const auto& change=history_[--cursor_];
        apply(change.uuid,change.before);
        return true;
    }
    bool redo() {
        if(cursor_==history_.size())return false;
        const auto& change=history_[cursor_++];
        apply(change.uuid,change.after);
        return true;
    }
    [[nodiscard]] bool canUndo() const noexcept {return cursor_>0;}
    [[nodiscard]] bool canRedo() const noexcept {return cursor_<history_.size();}
    void restore(Assignments assignments) {
        assignments_=std::move(assignments);
        history_.clear();cursor_=0;
    }
private:
    void apply(const std::string& uuid,const std::optional<SurfaceMedia>& value) {
        if(value)assignments_[uuid]=*value;
        else assignments_.erase(uuid);
    }
    Assignments assignments_;
    std::vector<Change> history_;
    std::size_t cursor_=0;
};
} // namespace voxen::ui
