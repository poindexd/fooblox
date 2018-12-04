//Awesomium
#include "application.h"
#include "view.h"
#include "method_dispatcher.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>

//RapidJSON
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

//OpenCV
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#undef _DEBUG /* Link with python24.lib and not python24_d.lib */
#include <Python.h>

#include <functional>
#include <iostream>
#include <fstream>
#include <regex>
#include <map>


#ifdef _WIN32
#include <Windows.h>
#endif


using namespace std;
using namespace Awesomium;
using namespace rapidjson;
using namespace cv;


class BlockDetector{
public:
	int param1 = 3;
	int param2 = 23;
	int min_radius = 34;
	int max_radius = 40;
	Mat src, gray, hsv;
	CvCapture* capture = 0;

	int s = 3;

	BlockDetector(){

		capture = cvCaptureFromCAM(1);

		namedWindow("Demo", CV_WINDOW_AUTOSIZE);
		namedWindow("Control", CV_WINDOW_FREERATIO);

		createTrackbar("s", "Control", &s, 5);
		createTrackbar("Param1", "Control", &param1, 10);
		createTrackbar("Param2", "Control", &param2, 50);
		createTrackbar("Min", "Control", &min_radius, 50);
		createTrackbar("Max", "Control", &max_radius, 100);

		initCodes();
	}

	vector<Vec3f> getCircles(){

		vector<Vec3f>  circles;

		src = cvQueryFrame(capture);
		flip(src, src, 0);
		flip(src, src, 1);

		cvtColor(src, hsv, COLOR_BGR2HSV);

		if (s != 0){
			erode(src, src, getStructuringElement(MORPH_ELLIPSE, Size(s, s)));
			dilate(src, src, getStructuringElement(MORPH_ELLIPSE, Size(s, s)));

			dilate(src, src, getStructuringElement(MORPH_ELLIPSE, Size(s, s)));
			erode(src, src, getStructuringElement(MORPH_ELLIPSE, Size(s, s)));
		}

		cvtColor(src, gray, CV_BGR2GRAY);
		GaussianBlur(gray, gray, Size(9, 9), 2, 2);

		HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, min_radius * 2, param1, param2, min_radius, max_radius);
		return circles;
	}

	//Gets the top/left most circle
	Vec3f getTopLeft(vector<Vec3f> circles){
		if (circles.size() < 1)
			return NULL;
		Vec3f min = circles[0];
		for (Vec3f circle : circles){
			if ((pow(circle[0], 2) + pow(circle[1], 2)) < (pow(min[0], 2) + pow(min[1], 2))){
				min = circle;
			}
		}
		return min;
		
	}

	string correctString(string s){
		s = regex_replace(s, regex(" {2,}"), " "); //Replace multispaces
		s = regex_replace(s, regex(" \\n"), "\n"); //Erase space before newline
		s = regex_replace(s, regex("for (.*?)\\n"), "for $1:\n"); //Add colon after for
		s = regex_replace(s, regex("\\n{2,}"), "\n");//replace multiple newlines
		s = regex_replace(s, regex("\\* \\*"), "**");
		s = regex_replace(s, regex("(.) (\\*\\*) (.)"), "$1$2$3");
		s = regex_replace(s, regex("(.) (\= \=) (.)"), "$1==$3"); //Replace = = with ==

		regex e("\print (.*?)\\n");
		return regex_replace(s, e, "print($1)\n");
	}

	string getInputString(){
		string code_string = "";
		try{
			vector<Vec3f> circles = getCircles();

			if (circles.size() <= 0)
				return "";

			Vec3f first_circle = getTopLeft(circles);

			Point center(first_circle[0], first_circle[1]);
			circle(src, center, 3, Scalar(255, 0, 0), -1, 8, 0);// Draw point on top/leftmost block.

			vector<vector<string>> code(10, vector<string>(10, ""));
			double width = first_circle[2] * (8.0 / 3.0);

			//Place blocks into array
			for (Vec3f circle : circles){
				int row = round((circle[1] - first_circle[1]) / width);
				int col = round((circle[0] - first_circle[0]) / width);
				if (row >=0 && col >=0)
				code[row][col] = getBlockCode(circle);
			}


			//Read array and build string

			for (int i = 0; i < code.size(); i++){
				for (int j = 0; j < code[i].size(); j++){
					code_string += code[i][j] + " ";
					if (code[i][j] == "")
						code[i][j] = "\t";
					else
						continue;
				}
				code_string += "\n";
			}

			imshow("Demo", src);
		}
		catch (Exception e){}

		return correctString(code_string);
	}

	vector<vector<string>> codes = vector<vector<string>>(7, vector<string>(7));
	const int RED = 0, ORANGE = 1, YELLOW = 2, GREEN = 3, BLUE = 4, WHITE = 5, PURPLE = 6;

	void initCodes(){
		//[Cube][Circle]

		//Operators
		codes[RED][RED] = "";
		codes[RED][ORANGE] = "-";
		codes[RED][YELLOW] = "*";
		codes[RED][GREEN] = "/";
		codes[RED][BLUE] = "%";
		codes[RED][WHITE] = "+";

		codes[ORANGE][RED] = "(";
		codes[ORANGE][ORANGE] = ")";
		codes[ORANGE][YELLOW] = "[";
		codes[ORANGE][GREEN] = "]";
		codes[ORANGE][BLUE] = "";
		//codes[ORANGE][WHITE] = ;

		//Control
		codes[YELLOW][RED] = "if";
		codes[YELLOW][ORANGE] = "else";
		codes[YELLOW][YELLOW] = "elif";
		codes[YELLOW][GREEN] = "while";
		codes[YELLOW][BLUE] = "in";
		codes[YELLOW][WHITE] = "for";

		codes[GREEN][RED] = "+";
		codes[GREEN][ORANGE] = "-";
		codes[GREEN][YELLOW] = "*";
		codes[GREEN][GREEN] = "/";
		codes[GREEN][BLUE] = "%";
		codes[GREEN][WHITE] = "print";

		//Variables
		codes[BLUE][RED] = "x";
		codes[BLUE][ORANGE] = "y";
		codes[BLUE][YELLOW] = "z";
		codes[BLUE][GREEN] = "i";
		codes[BLUE][BLUE] = "j";
		codes[BLUE][WHITE] = "k";

		//Comparison operators
		codes[PURPLE][RED] = "<";
		codes[PURPLE][ORANGE] = ">";
		codes[PURPLE][YELLOW] = "=";
		codes[PURPLE][GREEN] = "!";
		codes[PURPLE][BLUE] = "";
		codes[PURPLE][WHITE] = "";
	}

	int average(vector<int> vec){
		if (vec.size() == 0)
			return 0;
		int sum=0;
		for (int i = 0; i < vec.size(); i++){
			sum += vec[i];
		}
		return sum / vec.size();
	}

	Vec3b meanPixel(vector<Vec3b> pixels){
		vector<int> hues;
		vector<int> saturations;
		vector<int> values;
		for (int i = 0; i < pixels.size(); i++){
			hues.push_back(pixels[i][0]);
			saturations.push_back(pixels[i][1]);
			values.push_back(pixels[i][2]);
		}
		return Vec3b(average(hues), average(saturations), average(values));
	}

	int getStickerHue(Vec3f c){
		Point center(cvRound(c[0]), cvRound(c[1]));
		int radius = cvRound(c[2]);
		double big_radius = radius * 0.9;
		Point top = Point(center.x, center.y - big_radius);
		Vec3b pixel = hsv.at<Vec3b>(top);
		int hue = pixel[0];
		putText(src, to_string(hue), Point(center.x, center.y), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0, 255, 0), 2.0);
		return inColor(pixel);
	}

	int getEdgeHue(Vec3f c){
		Point center(cvRound(c[0]), cvRound(c[1]));
		int radius = cvRound(c[2]);
		double edge_radius = radius*1.15;
		Point edge(center.x - edge_radius, center.y);
		Vec3b pixel = hsv.at<Vec3b>(edge);
		int hue = pixel[0];
		putText(src, to_string(hue), Point(center.x, center.y+20), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0, 255, 0), 2.0);
		return outColor(pixel);
	}

	string getBlockCode(Vec3f c){
		string colors[] = { "RED", "ORANGE", "YELLOW", "GREEN", "BLUE", "WHITE", "PURPLE" };

		Point center(cvRound(c[0]), cvRound(c[1]));
		int radius = cvRound(c[2]);
		double big_radius = radius * 1.33;
		double small_radius = radius * 0.9;

		int center_hue = getStickerHue(c);
		int edge_hue = getEdgeHue(c);
		
		circle(src, center, 3, Scalar(0, 255, 0), -1, 8, 0);// circle center     
		circle(src, center, radius, Scalar(0, 0, 255), 3, 8, 0);// circle outline
		//circle(src, center, big_radius, Scalar(0, 255, 0), 3, 8, 0);// circle outline
		//circle(src, center, small_radius, Scalar(255, 120, 0), 3, 8, 0);// circle outline

		if (center_hue != -1)
			putText(src, colors[center_hue], Point(center.x, center.y - 40), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0, 255, 0), 2.0);
		if (edge_hue != -1)
			putText(src, colors[edge_hue], Point(center.x, center.y - 20), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0, 255, 0), 2.0);

		cout << "center : " << center << "\nradius : " << radius << endl;
		
		if (edge_hue >= 0 && center_hue >= 0)
			return codes[edge_hue][center_hue];
		else
			return "";
	}

	int outColor(Vec3b pixel){
		const int NUM_COLORS = 7;
		int colors[NUM_COLORS][2];
		colors[RED][0] = 173;
		colors[RED][1] = 6;

		colors[ORANGE][0] = 7;
		colors[ORANGE][1] = 15;

		colors[YELLOW][0] = 21;
		colors[YELLOW][1] = 34;

		colors[GREEN][0] = 56;
		colors[GREEN][1] = 96;

		colors[BLUE][0] = 106;
		colors[BLUE][1] = 121;

		colors[WHITE][0] = 97;
		colors[WHITE][1] = 107;

		colors[PURPLE][0] = 127;
		colors[PURPLE][1] = 171;

		for (int i = 0; i < NUM_COLORS; i++)
			if ((pixel.val[0] >= colors[i][0] && pixel.val[0] <= colors[i][1]
				|| i == RED && (pixel.val[0] >= colors[i][0] || pixel.val[0] <= colors[i][1])
				) && pixel.val[1] > 30 || (i == WHITE && pixel.val[1] <30)
				)
				return i;
		return -1;
	}

	int inColor(Vec3b pixel){
		const int NUM_COLORS = 7;
		int colors[NUM_COLORS][2];
		colors[RED][0] = 159;
		colors[RED][1] = 3;

		colors[ORANGE][0] = 4;
		colors[ORANGE][1] = 17;

		colors[YELLOW][0] = 21;
		colors[YELLOW][1] = 34;

		colors[GREEN][0] = 45;
		colors[GREEN][1] = 95;

		colors[BLUE][0] = 103;
		colors[BLUE][1] = 121;

		colors[WHITE][0] = 97;
		colors[WHITE][1] = 107;

		colors[PURPLE][0] = 124;
		colors[PURPLE][1] = 158;

		for (int i = 0; i < NUM_COLORS; i++)
			if ((pixel.val[0] >= colors[i][0] && pixel.val[0] <= colors[i][1]
				|| i == RED && (pixel.val[0] >= colors[i][0] || pixel.val[0] <= colors[i][1]))
				&& pixel.val[1] > 30
				|| (i == WHITE && pixel.val[1] <60)
				)
				return i;
		return -1;
	}

};

class Fooblox : public Application::Listener {
	Application* app_;
	View* view_;
	MethodDispatcher method_dispatcher_;
	WebView* web_view;
	BlockDetector b;
	int timer;
	//string current_level;
	string given;
	string correct;
	map<string, unsigned> last_strings;
	int current_level;
	boolean loaded;

public:
	Fooblox()
		: app_(Application::Create()),
		view_(0) {
		app_->set_listener(this);
		timer = 0;
	}


	virtual ~Fooblox() {
		if (view_)
			app_->DestroyView(view_);
		if (app_)
			delete app_;
	}

	void Run() {
		app_->Run();
	}

	// Inherited from Application::Listener
	virtual void Fooblox::OnLoaded();

	// Inherited from Application::Listener
	virtual void Fooblox::OnUpdate();

	// Inherited from Application::Listener
	virtual void Fooblox::OnShutdown();

	void loadLevel(int i);

	void BindMethods(WebView* web_view) {
		// Create a global js object named 'app'
		JSValue result = web_view->CreateGlobalJavascriptObject(WSLit("app"));
		if (result.IsObject()) {
			// Bind our custom method to it.
			JSObject& app_object = result.ToObject();
			method_dispatcher_.Bind(app_object,
				WSLit("sayHello"),
				JSDelegate(this, &Fooblox::OnSayHello));
		}

		// Bind our method dispatcher to the WebView
		web_view->set_js_method_handler(&method_dispatcher_);
	}

	// Bound to app.sayHello() in JavaScript
	void OnSayHello(WebView* caller,
		const JSArray& args) {
		app_->ShowMessage("Hello!");
	}
};

string fileToString(string path);
string runPython(string script);

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

namespace emb
{

	typedef std::function<void(std::string)> stdout_write_type;

	struct Stdout
	{
		PyObject_HEAD
			stdout_write_type write;
	};

	PyObject* Stdout_write(PyObject* self, PyObject* args)
	{
		std::size_t written(0);
		Stdout* selfimpl = reinterpret_cast<Stdout*>(self);
		if (selfimpl->write)
		{
			char* data;
			if (!PyArg_ParseTuple(args, "s", &data))
				return 0;

			std::string str(data);
			selfimpl->write(str);
			written = str.size();
		}
		return PyLong_FromSize_t(written);
	}

	PyObject* Stdout_flush(PyObject* self, PyObject* args)
	{
		// no-op
		return Py_BuildValue("");
	}

	PyMethodDef Stdout_methods[] =
	{
		{ "write", Stdout_write, METH_VARARGS, "sys.stdout.write" },
		{ "flush", Stdout_flush, METH_VARARGS, "sys.stdout.write" },
		{ 0, 0, 0, 0 } // sentinel
	};

	PyTypeObject StdoutType =
	{
		PyVarObject_HEAD_INIT(0, 0)
		"emb.StdoutType",     /* tp_name */
		sizeof(Stdout),       /* tp_basicsize */
		0,                    /* tp_itemsize */
		0,                    /* tp_dealloc */
		0,                    /* tp_print */
		0,                    /* tp_getattr */
		0,                    /* tp_setattr */
		0,                    /* tp_reserved */
		0,                    /* tp_repr */
		0,                    /* tp_as_number */
		0,                    /* tp_as_sequence */
		0,                    /* tp_as_mapping */
		0,                    /* tp_hash  */
		0,                    /* tp_call */
		0,                    /* tp_str */
		0,                    /* tp_getattro */
		0,                    /* tp_setattro */
		0,                    /* tp_as_buffer */
		Py_TPFLAGS_DEFAULT,   /* tp_flags */
		"emb.Stdout objects", /* tp_doc */
		0,                    /* tp_traverse */
		0,                    /* tp_clear */
		0,                    /* tp_richcompare */
		0,                    /* tp_weaklistoffset */
		0,                    /* tp_iter */
		0,                    /* tp_iternext */
		Stdout_methods,       /* tp_methods */
		0,                    /* tp_members */
		0,                    /* tp_getset */
		0,                    /* tp_base */
		0,                    /* tp_dict */
		0,                    /* tp_descr_get */
		0,                    /* tp_descr_set */
		0,                    /* tp_dictoffset */
		0,                    /* tp_init */
		0,                    /* tp_alloc */
		0,                    /* tp_new */
	};

	PyModuleDef embmodule =
	{
		PyModuleDef_HEAD_INIT,
		"emb", 0, -1, 0,
	};

	// Internal state
	PyObject* g_stdout;
	PyObject* g_stdout_saved;

	PyMODINIT_FUNC PyInit_emb(void)
	{
		g_stdout = 0;
		g_stdout_saved = 0;

StdoutType.tp_new = PyType_GenericNew;
if (PyType_Ready(&StdoutType) < 0)
	return 0;

PyObject* m = PyModule_Create(&embmodule);
if (m)
{
	Py_INCREF(&StdoutType);
	PyModule_AddObject(m, "Stdout", reinterpret_cast<PyObject*>(&StdoutType));
}
return m;
	}

	void set_stdout(stdout_write_type write)
	{
		if (!g_stdout)
		{
			g_stdout_saved = PySys_GetObject("stdout"); // borrowed
			g_stdout = StdoutType.tp_new(&StdoutType, 0, 0);
		}

		Stdout* impl = reinterpret_cast<Stdout*>(g_stdout);
		impl->write = write;
		PySys_SetObject("stdout", g_stdout);
	}

	void reset_stdout()
	{
		if (g_stdout_saved)
			PySys_SetObject("stdout", g_stdout_saved);

		Py_XDECREF(g_stdout);
		g_stdout = 0;
	}

} // namespace emb
