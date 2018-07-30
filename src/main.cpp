#include "lua.hpp"
#include "imgui.h"
#include "imgui-SFML.h"

#include "SFML\Graphics.hpp"

#include "filesys.h"
#include "limgui.h"
void load_projects(const char* prefix,file_watcher& fwatch)
{
    std::string path_prefix = prefix;
    auto pfiles = enum_files(path_prefix + "/*.lua");
    for (auto p : pfiles)
    {
        //TODO: add to project list
        watched_file f;
        f.path = path_prefix + "/" + p;
        fwatch.files.emplace_back(f);
    }
}

static int msghandler(lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg == NULL) {  /* is error object not a string? */
		if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
			lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
			return 1;  /* that is the message */
		else
			msg = lua_pushfstring(L, "(error object is a %s value)",
				luaL_typename(L, 1));
	}
	luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
	return 1;  /* return the traceback */
}

static int docall(lua_State *L, int narg, int nres) {
	int status;
	int base = lua_gettop(L) - narg;  /* function index */
	lua_pushcfunction(L, msghandler);  /* push message handler */
	lua_insert(L, base);  /* put it under function and args */
	status = lua_pcall(L, narg, nres, base);
	lua_remove(L, base);  /* remove message handler from the stack */
	return status;
}

struct project {
    lua_State *L=nullptr;
    std::string path;
    std::vector<std::string> errors;
	bool is_errored = false;
    project() {}
    ~project() { if(L)lua_close(L); };

    void init_lua() {
        if (L)
            lua_close(L);
        L = luaL_newstate();
        luaL_openlibs(L);
		lua_open_imgui(L);
		
		//lua_pushlightuserdata(L, this);
		//lua_setglobal(L, "__project");
    }
    void reload_file()
    {
        if (luaL_dofile(L, path.c_str()) != 0)
        {
            size_t len;
            const char* err=lua_tolstring(L, 1, &len);
            std::string error_str(err, len);
            errors.push_back(error_str);
			is_errored = true;
        }
		else
			is_errored = false;
    }
    void load_file(std::string file_path)
    {
        path = file_path;
        reload_file();
    }
    void clear_errors()
    {
        errors.clear();
		is_errored = false;
    }
    void reset()
    {
        clear_errors();
        init_lua();
        reload_file();
    }
	void update()
	{
		if (is_errored)
			return;
		lua_getglobal(L, "update");
		if (!lua_isnil(L, -1))
		{
			if (docall(L, 0, 0))
			{
				is_errored = true;
				errors.emplace_back(lua_tostring(L, -1));
			}
		}
		else
		{
			lua_pop(L, 1);
		}
		fixup_imgui_state();
	}
};
int main(int argc, char** argv)
{
    if (argc == 1)
    {
        printf("Usage: pixeldance.exe <path-to-projects>\n");
        return -1;
    }
    file_watcher fwatch;
    load_projects(argv[1], fwatch);

    sf::RenderWindow window(sf::VideoMode(1024, 1024), "PixelDance");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    project current_project;
    current_project.init_lua();
    int selected_project = -1;
    int old_selected = selected_project;
    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
		bool need_reload = false;
        if (fwatch.check_changes())
        {
            for (auto& f : fwatch.files)
            {
                if (f.changed && f.exists)
                {
                    //TODO: use changes
					if (current_project.path == f.path)
					{
						need_reload = true;
					}
                }
            }
        }
        ImGui::SFML::Update(window, deltaClock.restart());

		if(need_reload)
			current_project.reload_file();
		current_project.update();


        ImGui::ShowDemoWindow();
        ImGui::Begin("Projects");
        const char* project_name = "<no project>";
        if (selected_project >= 0 && selected_project < fwatch.files.size())
            project_name = fwatch.files[selected_project].path.c_str();

        if(ImGui::BeginCombo("Current project", project_name))
        {
            bool t = (selected_project==-1);
            ImGui::Selectable("<no project>", &t);
            if (t)
                selected_project = -1;
            int k = 0;
            for (auto& f : fwatch.files)
            {
                t = (selected_project == k);
                if(ImGui::Selectable(f.path.c_str(), &t))
                    selected_project = k;
                k++;
            }
            ImGui::EndCombo();
        }
        if (old_selected != selected_project)
        {
            if(selected_project!=-1)
            {
                current_project.load_file(fwatch.files[selected_project].path);
            }
        }
        old_selected = selected_project;
        ImGui::Separator();
        if (ImGui::Button("Clear"))
            current_project.clear_errors();

        ImGui::SameLine();
        if (ImGui::Button("Reset"))
            current_project.reset();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (auto& s : current_project.errors)
            ImGui::Text("%s", s.c_str());
        ImGui::EndChild();

        ImGui::Button("Test");
        ImGui::End();


        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }
    
    ImGui::SFML::Shutdown();

    return 0;
}