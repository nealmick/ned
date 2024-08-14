        #pragma once
        #include <string>
        #include <vector>
        #include <regex>
        #include <unordered_map>
        #include "imgui.h"
        #include <future>
        #include <atomic>
        #include <mutex>
        //

        struct SyntaxRule {
            std::regex pattern;
            ImVec4 color;
        };
        struct EditorState {
            int cursor_pos = 0;
            int selection_start = 0;
            int selection_end = 0;
            bool is_selecting = false; 
            std::vector<int> line_starts = {0};
            ImVec2 scroll_pos = ImVec2(0, 0); //vertical scroll
            float scroll_x = 0.0f;  // Horizontal scroll
            bool rainbow_cursor = true;
            float cursor_blink_time = 0.0f;
            bool activateFindBox = false;
            bool blockInput = false;
            int current_line;
            
        };
        struct ScrollChange {
            bool vertical;
            bool horizontal;
        };
        struct CursorVisibility {
            bool vertical;
            bool horizontal;
        };



        class Editor {
        public:
            void setLanguage(const std::string& extension);
            void highlightContent(const std::string& content, std::vector<ImVec4>& colors, int start_pos, int end_pos);
            void setTheme(const std::string& themeName);
            
            void moveWordForward(const std::string& text, EditorState& state);
            void moveWordBackward(const std::string& text, EditorState& state);
            void removeIndentation(std::string& text, EditorState& state);

            
        private:
            std::vector<SyntaxRule> rules;
            std::vector<SyntaxRule> htmlRules;
            std::vector<SyntaxRule> javascriptRules;
            std::vector<SyntaxRule> cssRules;
            std::vector<SyntaxRule> cppRules;
            std::vector<SyntaxRule> pythonRules;
            std::vector<SyntaxRule> markdownRules;
            std::vector<SyntaxRule> jsonRules;
            
            void setupCppRules();
            void setupPythonRules();
            void setupJavaScriptRules();
            void setupMarkdownRules();  
            void setupHtmlRules();      
            void setupCssRules(); 
            void setupJsonRules();
            
            void loadTheme(const std::string& themeName);
            std::unordered_map<std::string, ImVec4> themeColors;
            void applyRules(const std::string& view, std::vector<ImVec4>& colors, int start_pos, const std::vector<SyntaxRule>& rules);
            std::future<void> highlightFuture;
            std::atomic<bool> highlightingInProgress{false};
            std::mutex colorsMutex;
            
        };
        extern EditorState editor_state;
        extern Editor gEditor;


        inline ImVec4 GetRainbowColor(float t) {
            float r = sin(t) * 0.5f + 0.5f;
            float g = sin(t + 2.0944f) * 0.5f + 0.5f; // 2.0944 is 2π/3
            float b = sin(t + 4.1888f) * 0.5f + 0.5f; // 4.1888 is 4π/3
            return ImVec4(r, g, b, 1.0f);
        }


        void HandleEditorInput(const std::string& text, EditorState& state, const ImVec2& text_start_pos, float line_height, bool& text_changed, std::vector<ImVec4>& colors);
        bool CustomTextEditor(const char* label, std::string& text, std::vector<ImVec4>& colors, EditorState& editor_state);
        ScrollChange EnsureCursorVisible(const ImVec2& text_pos, const std::string& text, EditorState& state, float line_height, float window_height, float window_width);

