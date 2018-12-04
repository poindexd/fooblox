#include "main.h"

#ifdef _WIN32
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, wchar_t*, int nCmdShow) {
#else
int main() {
#endif

	Fooblox app;
	app.Run();

	return 0;
}

// Inherited from Application::Listener
void Fooblox::OnLoaded() {
	view_ = View::Create(500, 800);
	web_view = view_->web_view();

	BindMethods(web_view);

	WebURL url(WSLit("file:///C:/awesome/awesome/awesome/app.html"));
	web_view->LoadURL(url);
	current_level = 0;
	loadLevel(current_level);
}

const int CONSISTENCY = 4;

// Inherited from Application::Listener
void Fooblox::OnUpdate() {
	//if (!timer)

	while (!loaded){
		loadLevel(current_level);
	}

	string rendered;

	try{
		rendered = b.getInputString();
		last_strings[rendered]++;
	}
	catch (Exception e){ return; }

	if (timer == CONSISTENCY - 1){
		auto pr = std::max_element(last_strings.begin(), last_strings.end(),
			[](const pair<string, unsigned>& p1, const pair<string, unsigned>& p2) {
			return p1.second < p2.second; });
		rendered = pr->first;
		last_strings.clear();
		rendered = given + rendered;
		string output = runPython(rendered);
		JSValue window = web_view->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));

		if (window.IsObject()) {
			JSArray args, args2, args3;
			args.Push(WSLit(rendered.c_str()));
			args2.Push(WSLit(output.c_str()));
			args3.Push(WSLit(correct.c_str()));
			window.ToObject().Invoke(WSLit("updateRendered"), args);
			window.ToObject().Invoke(WSLit("updateOutput"), args2);

		}

		if (output == correct){
			if (window.IsObject()) {
				window.ToObject().Invoke(WSLit("levelCompleted"), JSArray());
			}
			current_level++;
			loadLevel(current_level);
		}

		timer = 0;
		return;
	}
	timer++;
}

// Inherited from Application::Listener
void Fooblox::OnShutdown() {
}

void Fooblox::loadLevel(int i){

	string json_string = fileToString("./Levels/0.json");
	const char* json = json_string.c_str();

	Document d;
	d.Parse(json);

	JSValue window = web_view->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));


	if (i >= 2){
		if (window.IsObject()) {
			window.ToObject().Invoke(WSLit("wonGame"), JSArray());
		}
		current_level = 0;
		loadLevel(current_level);
		return;
	}
	else {

		correct = runPython(d[i]["correct"].GetString());
		given = d[i]["given"].GetString();


		if (window.IsObject()) {
			JSArray args;
			args.Push(JSValue(i));
			args.Push(WSLit(d[i]["title"].GetString()));
			args.Push(WSLit(d[i]["objective"].GetString()));
			args.Push(WSLit(correct.c_str()));

			JSValue success = window.ToObject().Invoke(WSLit("loadLevel"), args);
			if (!success.IsBoolean())
				loaded = false;
			else
				loaded = true;
		}
	}
}

string fileToString(string path){
	ifstream in(path);
	string contents((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
	return contents;
}

string runPython(string script){
	PyImport_AppendInittab("main", emb::PyInit_emb);
	Py_Initialize();
	PyObject* main = PyImport_ImportModule("main");

	std::string buffer;
	{
		// switch sys.stdout to custom handler
		emb::stdout_write_type write = [&buffer](std::string s) { buffer += s; };
		emb::set_stdout(write);

		PyRun_SimpleString(script.c_str());

		emb::reset_stdout();
	}
	Py_Finalize();
	return buffer;
}