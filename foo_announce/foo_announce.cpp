// foo_announce.cpp : Defines the exported functions for the DLL application.
// I'd _almost_ forgotten the horror of coding anything for teh windoze.
// Now it's fresh in my mind.

/*
TODO (2013-02-23)

[ ] Escape control characters in JSON strings
[ ] Parse settings as a map and grab values somewhere else?
[ ] ...
*/
#include "stdafx.h"
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <memory>

// this is mandatory.
DECLARE_COMPONENT_VERSION("foo_announce", "0.2", "Foobar2000 track announcer");
// prevent renaming library or loading multiple different copies
VALIDATE_COMPONENT_FILENAME("foo_announce.dll");

extern cfg_string cfg_address;
extern cfg_string cfg_eventid;
extern cfg_string cfg_apikey;

WSADATA wsaData;
using dict = std::map<std::string, std::string>;


std::string escape_json_string(std::string str) {
	size_t p = 0;
	std::string res = str;
	while(p < res.length()) {
		if(res[p] == '"') {
			res.insert(p, "\\");
			p++;
		}
		p++;
	}
	return res;
}

/// Converts a flat dict into JSON with little care for encoding or anything :)
std::string bake_json(const dict &dict) {
	std::stringstream sstr;
	sstr << "{\r\n";
	dict::const_iterator it = dict.cbegin();
	while(it != dict.end())
	{
		sstr << "  \"" << escape_json_string(it->first) << "\": ";
		sstr << "\"" << escape_json_string(it->second) << "\"";
		it++;
		if(it != dict.cend())
		{
			sstr << ",";
		}
		sstr << "\r\n";
	}
	sstr << "}";
	return sstr.str();
}

/// Used to pass data to post_thread
struct post_params
{
	std::string hostname, port, path;
	dict dict;
};

void http_request_header(const char *method, const std::string &path, 
						 const dict &vars, std::stringstream &buf)
{
	buf << method << " " << path << " HTTP/1.1\r\n";
	dict::const_iterator it = vars.cbegin();
	while(it != vars.end()) {
		buf << it->first << ": " << it->second << "\r\n";
		it++;
	}
	buf << "\r\n";
}

/**
 * Asynchronicity!
 */
DWORD WINAPI post_thread(LPVOID params) {
	std::auto_ptr<post_params> p(reinterpret_cast<post_params*>(params));

	struct addrinfo *result = NULL, *ptr = NULL, hints;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int i_res = getaddrinfo(p->hostname.c_str(), p->port.c_str(), &hints, &result);
	if(i_res)
	{
		console::error("foo_announce: Failed to resolve announce server address!");
		return 0; // yes, nonzero = success according to msdn
	}
	SOCKET conn = INVALID_SOCKET;

	ptr = result;
	conn = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if(conn == INVALID_SOCKET)
	{
		console::error("foo_announce: Failed to init socket!");
		return 0;
	}

	i_res = connect(conn, ptr->ai_addr, (int)ptr->ai_addrlen);
	if(i_res == SOCKET_ERROR)
	{
		console::error("foo_announce: Failed to connect to announce server!");
		return 0;
	}
	freeaddrinfo(result);
	
	// Ok, maybe we can send something.
	std::string msg_str = bake_json(p->dict);
	std::stringstream header;

	dict vars;
	vars["Host"] = p->hostname + ":" + p->port;
	// cast to work around C++ defect 1261 (overloads required by standard are ambiguous)
	vars["Content-Length"] = std::to_string(static_cast<long long>(msg_str.length()));
	vars["User-Agent"] = "foo_announce";
	vars["Content-Type"] = "application/json";

	http_request_header("POST", p->path, vars, header);

	std::string header_str = header.str();
	i_res = send(conn, header_str.c_str(), header_str.length(), 0);
	i_res += send(conn, msg_str.c_str(), msg_str.length(), 0);

	if(!i_res)
	{
		console::error("foo_announce: Send failed!");
		shutdown(conn, SD_SEND);
		closesocket(conn);
	}

	// try to read reply
	i_res = 1;
	char buf[1024];
	std::stringstream reply;
	while(i_res > 0) {
		i_res = recv(conn, buf, 1024, 0);
		reply.write(buf, i_res);
	}
	// this API is thread-safe, right?
	console::info(reply.str().c_str());

	shutdown(conn, SD_SEND);
	closesocket(conn);
	return 1;
}

/**
 * Watches for new tracks and playback pausing.
 * Launches a thread to HTTP post any changes to the configured host.
 */
class playback_announcer : public play_callback_impl_base
{
public:
	void on_playback_new_track(metadb_handle_ptr p_track)
	{
		init_formatters();

		pfc::string_formatter out_title;
		p_track->format_title(NULL, out_title, fmt_title, NULL);
		pfc::string_formatter out_artist;
		p_track->format_title(NULL, out_artist, fmt_artist, NULL);

		dict msg;
		msg["type"] = "play";
		msg["title"] = out_title;
		msg["artist"] = out_artist;
		announce(msg);
	}
	void on_playback_stop(play_control::t_stop_reason p_reason)
	{
		if(p_reason == playback_control::stop_reason_starting_another
		   || p_reason == playback_control::stop_reason_eof)
		{
			// spamming these is probably unnecessary.
			return;
		}
		dict msg;
		msg["type"] = "stop";
		announce(msg);
	}
protected:
	titleformat_object::ptr fmt_title;
	titleformat_object::ptr fmt_artist;

	void announce(const dict &msg)
	{
		auto params = std::unique_ptr<post_params>(new post_params);
		params->dict = msg;
		params->dict["event_id"] = cfg_eventid.toString();
		params->dict["key"] = cfg_apikey.toString();
		
		// the address format is hostname[:port][/path/to/api]
		auto hoststring = cfg_address.toString();

		if (!hoststring || hoststring[0] == '\0') {
			console::error("Announcer unconfigured, not sending anything.");
			return;
		}

		int parsing_section = 0; // hostname, then port, then path
		size_t hostname_end = 0; // not inclusive
		size_t port_start = 0;
		size_t port_end = 0; // not inclusive
		size_t path_start = 0;
		int port = 0;
		size_t pos = 0;
		char c;

		while (c = hoststring[pos]) {
			// state machines, yay
			switch (parsing_section) {
				case 0: { // hostname, read characters until ':' or '/'
					if (c == ':') { // port incoming
						parsing_section = 1;
						hostname_end = pos;
						port_start = pos + 1;
					}
					else if (c == '/') {
						parsing_section = 2;
						hostname_end = path_start = pos;
					}
					break;

				}
				case 1: {
					if (c == '/') { // ports are followed by path
						port_end = pos;
						path_start = pos;
					}
					else {
					}
					break;
				}
			}
			pos++;
		}
		if (!hostname_end) {
			hostname_end = pos;
		}
		if (!port_end && port_start) {
			port_end = pos;
		}
		std::string str_hostname(hoststring, hostname_end);
		std::string str_port = (port_start > 0) ?
			std::string(hoststring + port_start, port_end - port_start) : "80";
		std::string str_path = (path_start > 0) ?
			std::string(hoststring + path_start, pos - path_start) : "/";
		
		params->hostname = str_hostname;
		params->port = str_port;
		params->path = str_path;
		CreateThread(NULL, 0, post_thread, params.release(), 0, NULL);
	}
	void init_formatters()
	{
		static_api_ptr_t<titleformat_compiler> compiler =
			static_api_ptr_t<titleformat_compiler>();

		if(fmt_title.is_empty()) {
			compiler->compile_safe_ex(fmt_title, "[%title%]");
			compiler->compile_safe_ex(fmt_artist, "[%artist%]");
		}
	}
};

/**
 * 
 */
class pba_initquit : public initquit
{
public:
	void on_init()
	{
		console::print("Initializing crappy playback announcer!");
		// Is this safe to do here?
		WSAStartup(MAKEWORD(2,2), &wsaData);
		new playback_announcer(); // captain, the hull is leaking
	}
	void on_quit() { }
};

static initquit_factory_t<pba_initquit> g_pba_initquit_factory;