#pragma once

//! Configuration of a UI element instance.
class NOVTABLE ui_element_config : public service_base {
public:
	//! Returns GUID of the UI element this configuration data belongs to.
	virtual GUID get_guid() const = 0;
	//! Returns raw configuration data pointer.
	virtual const void * get_data() const = 0;
	//! Returns raw configuration data size, in bytes.
	virtual t_size get_data_size() const = 0;


	//! Helper.
	static service_ptr_t<ui_element_config> g_create(const GUID& id, const void * data, t_size size);
	
	template<typename t_source> static service_ptr_t<ui_element_config> g_create(const GUID& id, t_source const & data) {
		pfc::assert_byte_type<typename t_source::t_item>();
		return g_create(id, data.get_ptr(), data.get_size());
	}

	static service_ptr_t<ui_element_config> g_create_empty(const GUID & id = pfc::guid_null) {return g_create(id,NULL,0);}

	static service_ptr_t<ui_element_config> g_create(const GUID & id, stream_reader * in, t_size bytes, abort_callback & abort);
	static service_ptr_t<ui_element_config> g_create(stream_reader * in, t_size bytes, abort_callback & abort);
	static service_ptr_t<ui_element_config> g_create(const void * data, t_size size);
	service_ptr_t<ui_element_config> clone() const {
		return g_create(get_guid(), get_data(), get_data_size());
	}

	FB2K_MAKE_SERVICE_INTERFACE(ui_element_config,service_base);
};

//! Helper for reading data from ui_element_config.
class ui_element_config_parser : public stream_reader_formatter<> {
public:
	ui_element_config_parser(ui_element_config::ptr in) : stream_reader_formatter(_m_stream, fb2k::noAbort), m_data(in), _m_stream(in->get_data(),in->get_data_size()) {}

	void reset() {_m_stream.reset();}
	t_size get_remaining() const {return _m_stream.get_remaining();}

	ui_element_config::ptr subelement(t_size size);
	ui_element_config::ptr subelement(const GUID & id, t_size dataSize);
private:
	const ui_element_config::ptr m_data;
	stream_reader_memblock_ref _m_stream;
};

//! Helper creating ui_element_config from your data.
class ui_element_config_builder : public stream_writer_formatter<> {
public:
	ui_element_config_builder() : stream_writer_formatter(_m_stream,fb2k::noAbort) {}
	ui_element_config::ptr finish(const GUID & id) {
		return ui_element_config::g_create(id,_m_stream.m_buffer);
	}
	void reset() {
		_m_stream.m_buffer.set_size(0);
	}
private:
	stream_writer_buffer_simple _m_stream;
};


FB2K_STREAM_WRITER_OVERLOAD(ui_element_config::ptr) {
	stream << value->get_guid();
	t_size size = value->get_data_size();
	stream << pfc::downcast_guarded<t_uint32>(size);
	stream.write_raw(value->get_data(),size);
	return stream;
}
FB2K_STREAM_READER_OVERLOAD(ui_element_config::ptr) {
	GUID guid;
	t_uint32 size;
	stream >> guid >> size;
	value = ui_element_config::g_create(guid,&stream.m_stream,size,stream.m_abort);
	return stream;
}


typedef uint32_t t_ui_color;
typedef fb2k::hfont_t t_ui_font;
typedef fb2k::hicon_t t_ui_icon;

constexpr GUID ui_color_text = { 0x5dd38be7, 0xff8a, 0x416f, { 0x88, 0x2d, 0xa4, 0x8e, 0x31, 0x87, 0x85, 0xb2 } };
constexpr GUID ui_color_background = { 0x16fc40c1, 0x1cba, 0x4385, { 0x93, 0x3b, 0xe9, 0x32, 0x7f, 0x6e, 0x35, 0x1f } };
constexpr GUID ui_color_highlight = { 0xd2f98042, 0x3e6a, 0x423a, { 0xb8, 0x66, 0x65, 0x1, 0xfe, 0xa9, 0x75, 0x93 } };
constexpr GUID ui_color_selection = { 0xebe1a36b, 0x7e0a, 0x469a, { 0x8e, 0xc5, 0xcf, 0x3, 0x12, 0x90, 0x40, 0xb5 } };
// Special pseudo-color - black or white depending on dark mode state, undefined in fb2k versions that don't recognize dark mode.
constexpr GUID ui_color_darkmode = { 0x9050bca9, 0x4ed, 0x40e9, { 0xb0, 0xfc, 0x9c, 0x67, 0x9c, 0xc2, 0x28, 0x6d } };


constexpr GUID ui_font_default = { 0x9ef02cef, 0xe58a, 0x4f99, { 0x9f, 0xe3, 0x85, 0x39, 0xb, 0xed, 0xc5, 0xe0 } };
constexpr GUID ui_font_tabs = { 0x65ffd7ac, 0x71ce, 0x495c, { 0xa5, 0x99, 0x48, 0x5b, 0xbf, 0x7a, 0x4, 0x7b } };
constexpr GUID ui_font_lists = { 0xa86198b3, 0xb5d8, 0x40cf, { 0xac, 0x19, 0xf9, 0xda, 0xc, 0xb5, 0xd4, 0x24 } };
constexpr GUID ui_font_playlists = { 0xd85b7b7e, 0xbf83, 0x41ed, { 0x88, 0xce, 0x1, 0x99, 0x31, 0x94, 0x3e, 0x2f } };
constexpr GUID ui_font_statusbar = { 0xc7fd555b, 0xbd15, 0x4f74, { 0x93, 0xe, 0xba, 0x55, 0x52, 0x9d, 0xd9, 0x71 } };
constexpr GUID ui_font_console = { 0xb08c619d, 0xd3d1, 0x4089, { 0x93, 0xb2, 0xd5, 0xb, 0x87, 0x2d, 0x1a, 0x25 } };


#ifdef _WIN32

//! @returns -1 when the GUID is unknown / unmappable, index that can be passed over to GetSysColor() otherwise.
int ui_color_to_sys_color_index(const GUID & p_guid);
GUID ui_color_from_sys_color_index( int idx );

struct ui_element_min_max_info {
	t_uint32 m_min_width = 0, m_max_width = UINT32_MAX, m_min_height = 0, m_max_height = UINT32_MAX;

	const ui_element_min_max_info & operator|=(const ui_element_min_max_info & p_other);
	ui_element_min_max_info operator|(const ui_element_min_max_info & p_other) const;
	void adjustForWindow(HWND wnd);
};

typedef SIZE ui_size;
typedef RECT ui_rect;
#else
struct ui_element_min_max_info {}; // dummy
struct ui_size { int cx = 0, cy = 0; };
struct ui_rect { int l = 0, t = 0, r = 0, b = 0; };
#endif

//! Callback class passed by a UI element host to a UI element instance, allowing each UI element instance to communicate with its host. \n
//! Each ui_element_instance_callback implementation must also implement ui_element_instance_callback_v2.
class NOVTABLE ui_element_instance_callback : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(ui_element_instance_callback,service_base);
public:
	virtual void on_min_max_info_change() = 0;
	//! Deprecated, does nothing.
	virtual void on_alt_pressed(bool p_state) = 0;
	//! Returns true on success, false when the color is undefined and defaults such as global windows settings should be used.
	virtual bool query_color(const GUID & p_what,t_ui_color & p_out) = 0;
	//! Tells the host that specified element wants to activate itself as a result of some kind of external user command (eg. menu command). Host should ensure that requesting child element's window is visible.\n
	//! @returns True when relevant child element window has been properly made visible and requesting code should proceed to SetFocus their window etc, false when denied.
	virtual bool request_activation(service_ptr_t<class ui_element_instance> p_item) = 0;

	//! Queries whether "edit mode" is enabled. Most of UI element editing functionality should be locked when it's not.
	virtual bool is_edit_mode_enabled() = 0;
	
	//! Tells the host that the user has requested the element to be replaced with another one.
	//! Note: this is generally used only when "edit mode" is enabled, but legal to call when it's not (eg. dummy element calls it regardless of settings when clicked).
	virtual void request_replace(service_ptr_t<class ui_element_instance> p_item) = 0;

	//! Deprecated - use query_font_ex. Equivalent to query_font_ex(ui_font_default).
	t_ui_font query_font() {return query_font_ex(ui_font_default);}

	//! Retrieves an user-configurable font to use for specified kind of display. See ui_font_* constant for possible parameters.
	virtual t_ui_font query_font_ex(const GUID & p_what) = 0;

	//! Helper - a wrapper around query_color(), if the color is not user-overridden, returns relevant system color.
	t_ui_color query_std_color(const GUID & p_what);
#ifdef _WIN32
	t_ui_color getSysColor( int sysColorIndex );
#endif

	bool is_elem_visible_(service_ptr_t<class ui_element_instance> elem);

	t_size notify_(ui_element_instance * source, const GUID & what, t_size param1, const void * param2, t_size param2size);
	bool set_elem_label(ui_element_instance * source, const char * label);
	t_uint32 get_dialog_texture(ui_element_instance * source);
	bool is_border_needed(ui_element_instance * source);

	bool is_dark_mode();
};


class NOVTABLE ui_element_instance_callback_v2 : public ui_element_instance_callback {
	FB2K_MAKE_SERVICE_INTERFACE(ui_element_instance_callback_v2, ui_element_instance_callback);
public:
	virtual bool is_elem_visible(service_ptr_t<class ui_element_instance> elem) = 0;	
};

class NOVTABLE ui_element_instance_callback_v3 : public ui_element_instance_callback_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(ui_element_instance_callback_v3, ui_element_instance_callback_v2)
public:
	//! Returns zero when the notification was not handled, return value depends on the notification otherwise.
	virtual t_size notify(ui_element_instance * source, const GUID & what, t_size param1, const void * param2, t_size param2size) = 0;
};

typedef service_ptr_t<ui_element_instance_callback> ui_element_instance_callback_ptr;

//! ui_element_instance_callback implementation helper.
template<typename t_receiver> class ui_element_instance_callback_impl : public ui_element_instance_callback_v3 {
public:
	ui_element_instance_callback_impl(t_receiver * p_receiver) : m_receiver(p_receiver) {}
	void on_min_max_info_change() {
		if (m_receiver != NULL) m_receiver->on_min_max_info_change();
	}
	void on_alt_pressed(bool p_state) { (void)p_state; }

	bool query_color(const GUID & p_what,t_ui_color & p_out) {
		if (m_receiver != NULL) return m_receiver->query_color(p_what,p_out);
		else return false;
	}

	bool request_activation(service_ptr_t<class ui_element_instance> p_item) {
		if (m_receiver) return m_receiver->request_activation(p_item);
		else return false;
	}

	bool is_edit_mode_enabled() {
		if (m_receiver) return m_receiver->is_edit_mode_enabled();
		else return false;
	}
	void request_replace(service_ptr_t<class ui_element_instance> p_item) {
		if (m_receiver) m_receiver->request_replace(p_item);
	}

	t_ui_font query_font_ex(const GUID & p_what) {
		if (m_receiver) return m_receiver->query_font_ex(p_what);
		else return NULL;
	}

	void orphan() {m_receiver = NULL;}

	bool is_elem_visible(service_ptr_t<class ui_element_instance> elem) {
		if (m_receiver) return m_receiver->is_elem_visible(elem);
		else return false;
	}
	t_size notify(ui_element_instance * source, const GUID & what, t_size param1, const void * param2, t_size param2size) {
		if (m_receiver) return m_receiver->host_notify(source, what, param1, param2, param2size);
		else return 0;
	}
private:
	t_receiver * m_receiver;
};

//! ui_element_instance_callback implementation helper.
class ui_element_instance_callback_receiver {
public:
	virtual void on_min_max_info_change() {}
	virtual bool query_color(const GUID& p_what, t_ui_color& p_out) { (void)p_what; (void)p_out; return false; }
	virtual bool request_activation(service_ptr_t<class ui_element_instance> p_item) { (void)p_item; return false; }
	virtual bool is_edit_mode_enabled() {return false;}
	virtual void request_replace(service_ptr_t<class ui_element_instance> p_item) {}
	virtual t_ui_font query_font_ex(const GUID&) {return NULL;}
	virtual bool is_elem_visible(service_ptr_t<ui_element_instance> elem) {return true;}
	virtual t_size host_notify(ui_element_instance* source, const GUID& what, t_size param1, const void* param2, t_size param2size) { (void)source; (void)what; (void)param1; (void)param2; (void)param2size; return 0; }
	ui_element_instance_callback_ptr ui_element_instance_callback_get_ptr() {
		if (m_callback.is_empty()) m_callback = new service_impl_t<t_callback>(this);
		return m_callback;
	}
	void ui_element_instance_callback_release() {
		if (m_callback.is_valid()) {
			m_callback->orphan();
			m_callback.release();
		}
	}
protected:
	~ui_element_instance_callback_receiver() {
		ui_element_instance_callback_release();		
	}
	ui_element_instance_callback_receiver() {}

private:
	typedef ui_element_instance_callback_receiver t_self;
	typedef ui_element_instance_callback_impl<t_self> t_callback;
	service_ptr_t<t_callback> m_callback;
};

//! Instance of a UI element.
class NOVTABLE ui_element_instance : public service_base {
public:
	//! Returns ui_element_instance's window handle.\n
	//! UI Element's window must be created when the ui_element_instance object is created. The window may or may not be destroyed by caller before the ui_element_instance itself is destroyed. If caller doesn't destroy the window before ui_element_instance destruction, ui_element_instance destructor should do it.
	virtual fb2k::hwnd_t get_wnd() = 0;
	
	//! Alters element's current configuration. Specified ui_element_config's GUID must be the same as this element's GUID.
	virtual void set_configuration(ui_element_config::ptr data) = 0;
	//! Retrieves element's current configuration. Returned object's GUID must be set to your element's GUID so your element can be re-instantiated with stored settings.
	virtual ui_element_config::ptr get_configuration() = 0;

	//! Returns GUID of the element. The return value must be the same as your ui_element::get_guid().
	virtual GUID get_guid() = 0;
	//! Returns subclass GUID of the element. The return value must be the same as your ui_element::get_subclass().
	virtual GUID get_subclass() = 0;

	//! Returns element's focus priority.
	virtual double get_focus_priority() {return 0;}
	//! Elements that host other elements should pass the call to the child with the highest priority. Override for container elements.
	virtual void set_default_focus() {set_default_focus_fallback();}

	//! Overridden by containers only.
	virtual bool get_focus_priority_subclass(double & p_out,const GUID & p_subclass) {
		if (p_subclass == get_subclass()) {p_out = get_focus_priority(); return true;}
		else {return false;}
	}
	//! Overridden by containers only.
	virtual bool set_default_focus_subclass(const GUID & p_subclass) {
		if (p_subclass == get_subclass()) {set_default_focus(); return true;}
		else {return false;}
	}

	//! Retrieves element's minimum/maximum window size. Default implementation will fall back to WM_GETMINMAXINFO.
	virtual ui_element_min_max_info get_min_max_info();

	//! Used by host to notify the element about various events. See ui_element_notify_* GUIDs for possible p_what parameter; meaning of other parameters depends on p_what value. Container classes should dispatch all notifications to their children.
	virtual void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) { (void)p_what; (void)p_param1; (void)p_param2; (void)p_param2size; }

#ifdef _WIN32
	//! @param p_point Context menu point in screen coordinates. Always within out window's rect.
	//! @return True to request edit_mode_context_menu_build() call to add our own items to the menu, false if we can't supply a context menu for this point.
	virtual bool edit_mode_context_menu_test(const POINT& p_point, bool p_fromkeyboard) { (void)p_point; (void)p_fromkeyboard; return false; }
	virtual void edit_mode_context_menu_build(const POINT& p_point, bool p_fromkeyboard, HMENU p_menu, unsigned p_id_base) { (void)p_point; (void)p_fromkeyboard; (void)p_menu; (void)p_id_base; }
	virtual void edit_mode_context_menu_command(const POINT& p_point, bool p_fromkeyboard, unsigned p_id, unsigned p_id_base) { (void)p_point; (void)p_fromkeyboard; (void)p_id; (void)p_id_base; }
	//! @param p_point Receives the point to spawn context menu over when user has pressed the context menu key; in screen coordinates.
	virtual bool edit_mode_context_menu_get_focus_point(POINT& p_point) { (void)p_point; return false; }

	virtual bool edit_mode_context_menu_get_description(unsigned p_id, unsigned p_id_base, pfc::string_base& p_out) { (void)p_id; (void)p_id_base; (void)p_out; return false; }
#endif // _WIN32

    //! Helper.
    void set_default_focus_fallback() {
#ifdef _WIN32
        const HWND thisWnd = this->get_wnd();
        if (thisWnd != NULL) ::SetFocus(thisWnd);
#endif
    }


	FB2K_MAKE_SERVICE_INTERFACE(ui_element_instance,service_base);
};

typedef service_ptr_t<ui_element_instance> ui_element_instance_ptr;


//! Interface to enumerate and possibly alter children of a container element. Obtained from ui_element::enumerate_children().
class NOVTABLE ui_element_children_enumerator : public service_base {
public:
	//! Retrieves the container's children count.
	virtual t_size get_count() = 0;
	//! Retrieves the configuration of the child at specified index, 0 <= index < get_count().
	virtual ui_element_config::ptr get_item(t_size p_index) = 0;

	//! Returns whether children count can be altered. For certain containers, children count is fixed and this method returns false.
	virtual bool can_set_count() = 0;
	//! Alters container's children count (behavior is undefined when can_set_count() returns false). Newly allocated children get default empty element configuration.
	virtual void set_count(t_size count) = 0;
	//! Alters the selected item.
	virtual void set_item(t_size p_index,ui_element_config::ptr cfg) = 0;
	//! Creates a new ui_element_config for this container, with our changes (set_count(), set_item()) applied.
	virtual ui_element_config::ptr commit() = 0;

	FB2K_MAKE_SERVICE_INTERFACE(ui_element_children_enumerator,service_base);
};


typedef service_ptr_t<ui_element_children_enumerator> ui_element_children_enumerator_ptr;


//! Entrypoint interface for each UI element implementation.
class NOVTABLE ui_element : public service_base {
public:
	//! Retrieves GUID of the element.
	virtual GUID get_guid() = 0;

	//! Retrieves subclass GUID of the element. Typically one of ui_element_subclass_* values, but you can create your own custom subclasses. Subclass GUIDs are used for various purposes such as focusing an UI element that provides specific functionality.
	virtual GUID get_subclass() = 0;
	
	//! Retrieves a simple human-readable name of the element.
	virtual void get_name(pfc::string_base & p_out) = 0;

	//! Instantiates the element using specified settings.
	virtual ui_element_instance_ptr instantiate(fb2k::hwnd_t p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback) = 0;

	//! Retrieves default configuration of the element.
	virtual ui_element_config::ptr get_default_configuration() = 0;

	//! Implemented by container elements only. Returns NULL for non-container elements. \n
	//! Allows caller to parse and edit child element structure of container elements.
	virtual ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) = 0;

	
	//! In certain cases, an UI element can import settings of another UI element (eg. vertical<=>horizontal splitter, tabs<=>splitters) when user directly replaces one of such elements with another. Overriding this function allows special handling of such cases. \n
	//! Implementation hint: when implementing a multi-child container, you probably want to takeover child elements replacing another container element; use enumerate_children() on the element the configuration belongs to to grab those.
	//! @returns A new ui_element_config on success, a null pointer when the input data could not be parsed / is in an unknown format.
	virtual ui_element_config::ptr import(ui_element_config::ptr cfg) {return NULL;}

	//! Override this to return false when your element is for internal use only and should not be user-addable.
	virtual bool is_user_addable() {return true;}

	//! Returns an icon to show in available UI element lists, etc. Returns NULL when there is no icon to display.
	virtual t_ui_icon get_icon() {return NULL;}
	
	//! Retrieves a human-readable description of the element.
	virtual bool get_description(pfc::string_base& p_out) { (void)p_out; return false; }

	//! Retrieves a human-readable description of the element's function to use for grouping in the element list. The default implementation relies on get_subclass() return value.
	virtual bool get_element_group(pfc::string_base & p_out);

	static bool g_find(service_ptr_t<ui_element> & out, const GUID & id);
	static bool g_get_name(pfc::string_base & p_out,const GUID & p_guid);

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(ui_element);
};

//! Extended interface for a UI element implementation.
class NOVTABLE ui_element_v2 : public ui_element {
public:
	enum {
		//! Indicates that bump() method is supported.
		KFlagSupportsBump		= 1 << 0,
		//! Tells UI backend to auto-generate a menu command activating your element - bumping an existing instance if possible, spawning a popup otherwise.
		//! Currently menu commands are generated for ui_element_subclass_playback_visualisation, ui_element_subclass_media_library_viewers and ui_element_subclass_utility subclasses, in relevant menus.
		KFlagHavePopupCommand	= 1 << 1,
		//! Tells backend that your element supports fullscreen mode (typically set only for visualisations).
		KFlagHaveFullscreen		= 1 << 2,

		KFlagPopupCommandHidden = 1 << 3,

		KFlagsVisualisation = KFlagHavePopupCommand | KFlagHaveFullscreen,
	};
	virtual t_uint32 get_flags() = 0;
	//! Called only when get_flags() return value has KFlagSupportsBump bit set. 
	//! Returns true when an existing instance of this element has been "bumped" - brought to user's attention in some way, false when there's no instance to bump or none of existing instances could be bumped for whatever reason.
	virtual bool bump() = 0;

	//! Override to use another GUID for our menu command. Relevant only when KFlagHavePopupCommand is set.
	virtual GUID get_menu_command_id() {return get_guid();}

	//! Override to use another description for our menu command. Relevant only when KFlagHavePopupCommand is set.
	virtual bool get_menu_command_description(pfc::string_base & out) {
		pfc::string8 name; get_name(name);
		out = PFC_string_formatter() << "Activates " << name << " window.";
		return true;
	}

	//! Override to use another name for our menu command. Relevant only when KFlagHavePopupCommand is set.
	virtual void get_menu_command_name(pfc::string_base & out) {get_name(out);}
    
	//! Relevant only when KFlagHavePopupCommand is set.
	//! @param defSize Default window size @ 96DPI. If screen DPI is different, it will be rescaled appropriately.
	//! @param title Window title to use.
	//! @returns True when implemented, false when not - caller will automatically determine default size and window title then.
	virtual bool get_popup_specs(ui_size& defSize, pfc::string_base& title) { (void)defSize; (void)title; return false; }

	
	FB2K_MAKE_SERVICE_INTERFACE(ui_element_v2, ui_element)
};

class NOVTABLE ui_element_popup_host_callback : public service_base {
public:
	virtual void on_resize(t_uint32 width, t_uint32 height) = 0;
	virtual void on_close() = 0;
	virtual void on_destroy() = 0;

	FB2K_MAKE_SERVICE_INTERFACE(ui_element_popup_host_callback,service_base);
};

class NOVTABLE ui_element_popup_host : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(ui_element_popup_host,service_base);
public:
	virtual ui_element_instance::ptr get_root_elem() = 0;
	virtual fb2k::hwnd_t get_wnd() = 0;
	virtual ui_element_config::ptr get_config() = 0;
	virtual void set_config(ui_element_config::ptr cfg) = 0;
	//! Sets edit mode on/off. Default: off.
	virtual void set_edit_mode(bool state) = 0;
};

class NOVTABLE ui_element_popup_host_v2 : public ui_element_popup_host {
	FB2K_MAKE_SERVICE_INTERFACE(ui_element_popup_host_v2, ui_element_popup_host);
public:
	virtual void set_window_title(const char * title) = 0;
	virtual void allow_element_specified_title(bool allow) = 0;
};

//! Shared implementation of common UI Element methods. Use ui_element_common_methods::get() to obtain an instance.
class NOVTABLE ui_element_common_methods : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(ui_element_common_methods);
public:
	virtual void copy(ui_element_config::ptr cfg) = 0;
	virtual void cut(ui_element_instance_ptr & p_instance,fb2k::hwnd_t p_parent,ui_element_instance_callback_ptr p_callback) = 0;
	virtual bool paste(ui_element_instance_ptr & p_instance,fb2k::hwnd_t p_parent,ui_element_instance_callback_ptr p_callback) = 0;
	virtual bool is_paste_available() = 0;
	virtual bool paste(ui_element_config::ptr & out) = 0;

#ifdef _WIN32
	virtual bool parse_dataobject_check(pfc::com_ptr_t<IDataObject> in, DWORD & dropEffect) = 0;
	virtual bool parse_dataobject(pfc::com_ptr_t<IDataObject> in,ui_element_config::ptr & out, DWORD & dropEffect) = 0;

	virtual pfc::com_ptr_t<IDataObject> create_dataobject(ui_element_config::ptr in) = 0;
#endif
    
	virtual fb2k::hwnd_t spawn_scratchbox(fb2k::hwnd_t parent,ui_element_config::ptr cfg) = 0;

	virtual ui_element_popup_host::ptr spawn_host(fb2k::hwnd_t parent, ui_element_config::ptr cfg, ui_element_popup_host_callback::ptr callback, ui_element::ptr elem = NULL
#ifdef _WIN32
    ,DWORD style = WS_POPUPWINDOW|WS_CAPTION|WS_THICKFRAME, DWORD styleEx = WS_EX_CONTROLPARENT
#endif
    ) = 0;

	void copy(ui_element_instance_ptr p_instance) {copy(p_instance->get_configuration());}

	
};

//! Shared implementation of common UI Element methods. Use ui_element_common_methods_v2::get() to obtain an instance.
class NOVTABLE ui_element_common_methods_v2 : public ui_element_common_methods {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(ui_element_common_methods_v2, ui_element_common_methods);
public:
	virtual void spawn_host_simple(fb2k::hwnd_t parent, ui_element::ptr elem, bool fullScreenMode) = 0;

	void spawn_host_simple(fb2k::hwnd_t parent, const GUID & elem, bool fullScreenMode) {
		spawn_host_simple(parent, service_by_guid<ui_element>(elem), fullScreenMode);
	}

	virtual void toggle_fullscreen(ui_element::ptr elem, fb2k::hwnd_t parent) = 0;
	
	void toggle_fullscreen(const GUID & elem, fb2k::hwnd_t parent) {
		toggle_fullscreen(service_by_guid<ui_element>(elem), parent);
	}
};

//! \since 1.4
//! Callback class for "Replace UI Element" dialog.
class NOVTABLE ui_element_replace_dialog_notify : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(ui_element_replace_dialog_notify, service_base);
public:
	virtual void on_cancelled() = 0;
	virtual void on_ok( const GUID & guid ) = 0;

	//! Helper; reply is called with new elem GUID on OK and with a null GUID on cancel.
	static ui_element_replace_dialog_notify::ptr create( std::function<void (GUID)> reply );
};

//! \since 1.4
class NOVTABLE ui_element_common_methods_v3 : public ui_element_common_methods_v2 {
public:
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(ui_element_common_methods_v3, ui_element_common_methods_v2);
public:
	//! Creates a "Replace UI Element" or "Add New UI Element" dialog.
	//! @param wndElem Parent *element* window handle, the dialog will be a child of its parent popup window but centered on top of the specified window.
	//! @param elemReplacing GUID of element being replaced; specify null to show "Add UI Element" dialog.
	//! @param notify Callback object receiving OK/Cancel notifications.
	//! @returns Handle to the newly created dialog. You can just destroy this window if you need to abort the dialog programatically.
	virtual fb2k::hwnd_t replace_element_dialog_start(fb2k::hwnd_t wndElem, const GUID & elemReplacing, ui_element_replace_dialog_notify::ptr notify) = 0;
	
	//! Highlights the element, creating an overlay window above it. Caller is responsible for destroying the overlay.
	virtual fb2k::hwnd_t highlight_element( fb2k::hwnd_t wndElem ) = 0;
};

//! Dispatched through ui_element_instance::notify() when host changes color settings. Other parameters are not used and should be set to zero.
constexpr GUID ui_element_notify_colors_changed = { 0xeedda994, 0xe3d2, 0x441a, { 0xbe, 0x47, 0xa1, 0x63, 0x5b, 0x71, 0xab, 0x60 } };
constexpr GUID ui_element_notify_font_changed = { 0x7a6964a8, 0xc797, 0x4737, { 0x90, 0x55, 0x7d, 0x84, 0xe7, 0x3d, 0x63, 0x6e } };
constexpr GUID ui_element_notify_edit_mode_changed = { 0xf72f00af, 0xec76, 0x4251, { 0xb2, 0x67, 0x89, 0x4c, 0x52, 0x5f, 0x18, 0xc6 } };

//! Sent when a portion of the GUI is shown/hidden. First parameter is a bool flag indicating whether your UI Element is now visible.
constexpr GUID ui_element_notify_visibility_changed = { 0x313c22b9, 0x287a, 0x4804, { 0x8e, 0x6c, 0xff, 0xef, 0x4, 0x10, 0xcd, 0xea } };

//! Sent to retrieve areas occupied by elements to show overlay labels. Param1 is ignored, param2 is a pointer to ui_element_notify_get_element_labels_callback, param2size is ignored.
constexpr GUID ui_element_notify_get_element_labels = { 0x4850a2cb, 0x6cfc, 0x4d74, { 0x9a, 0x6d, 0xc, 0x7b, 0x29, 0x16, 0x5e, 0x69 } };


//! Set to ui_element_instance_callback_v3 to set our element's label. Param1 is ignored, param2 is a pointer to a UTF-8 string containing the new label. Return value is 1 if the label is user-visible, 0 if the host does not support displaying overridden labels.
constexpr GUID ui_element_host_notify_set_elem_label = { 0x24598cb7, 0x9c5c, 0x488e, { 0xba, 0xff, 0xd, 0x2f, 0x11, 0x93, 0xe4, 0xf2 } };
constexpr GUID ui_element_host_notify_get_dialog_texture = { 0xbb98eb99, 0x7b07, 0x42f3, { 0x8c, 0xd1, 0x12, 0xa2, 0xc2, 0x23, 0xb5, 0xbc } };
constexpr GUID ui_element_host_notify_is_border_needed = { 0x2974f554, 0x2f31, 0x49c5, { 0xab, 0x4, 0x76, 0x4a, 0xf7, 0x94, 0x7c, 0x4f } };



class ui_element_notify_get_element_labels_callback {
public:
	virtual void set_area_label(const ui_rect & rc, const char * name) = 0;
	virtual void set_visible_element(ui_element_instance::ptr item) = 0;
protected:
	ui_element_notify_get_element_labels_callback() {}
	~ui_element_notify_get_element_labels_callback() {}

	PFC_CLASS_NOT_COPYABLE_EX(ui_element_notify_get_element_labels_callback);
};


constexpr GUID ui_element_subclass_playlist_renderers = { 0x3c4c68a0, 0xfc5, 0x400a, { 0xa3, 0x4a, 0x2e, 0x3a, 0xae, 0x6e, 0x90, 0x76 } };
constexpr GUID ui_element_subclass_media_library_viewers = { 0x58455355, 0x289d, 0x459c, { 0x8f, 0x8a, 0xe1, 0x49, 0x6, 0xfc, 0x14, 0x56 } };
constexpr GUID ui_element_subclass_containers = { 0x72dc5954, 0x1f26, 0x41be, { 0xae, 0xf2, 0x92, 0x9d, 0x25, 0xb5, 0x8d, 0xcf } };
constexpr GUID ui_element_subclass_selection_information = { 0x68084e43, 0x7359, 0x46a5, { 0xb6, 0x84, 0x3c, 0xd7, 0x57, 0xf6, 0xde, 0xfd } };
constexpr GUID ui_element_subclass_playback_visualisation = { 0x1f3c62f2, 0x8bb5, 0x4700, { 0x9e, 0x82, 0x8c, 0x48, 0x22, 0xf0, 0x18, 0x35 } };
constexpr GUID ui_element_subclass_playback_information = { 0x84859f2d, 0xbb9c, 0x4e70, { 0x9d, 0x4, 0x14, 0x71, 0xb5, 0x63, 0x1f, 0x7f } };
constexpr GUID ui_element_subclass_utility = { 0xffa4f4fc, 0xc169, 0x4766, { 0x9c, 0x94, 0xfa, 0xef, 0xae, 0xb2, 0x7e, 0xf } };
constexpr GUID ui_element_subclass_dsp = { 0xa6a93251, 0xf0f8, 0x4bed,{ 0xb9, 0x5a, 0xf9, 0xe, 0x7e, 0x4f, 0xf2, 0xd0 } };


bool ui_element_subclass_description(const GUID & id, pfc::string_base & out);


#define ReplaceUIElementCommand "Replace UI Element..."
#define ReplaceUIElementDescription "Replaces this UI Element with another one."

#define CopyUIElementCommand "Copy UI Element"
#define CopyUIElementDescription "Copies this UI Element to Windows Clipboard."

#define PasteUIElementCommand "Paste UI Element"
#define PasteUIElementDescription "Replaces this UI Element with Windows Clipboard content."

#define CutUIElementCommand "Cut UI Element"
#define CutUIElementDescription "Copies this UI Element to Windows Clipboard and replaces it with an empty UI Element."

#define AddNewUIElementCommand "Add New UI Element..."
#define AddNewUIElementDescription "Replaces the selected empty space with a new UI Element."

//! \since 2.0
class NOVTABLE ui_config_callback {
public:
	//! Called when user changes configuration of fonts.
	virtual void ui_fonts_changed() {}
	//! Called when user changes configuration of colors (also as a result of toggling dark mode). \n
	//! Note that for the duration of these callbacks, both old handles previously returned by query_font() as well as new ones are valid; old font objects are released when the callback cycle is complete.
	virtual void ui_colors_changed() {}
};

//! \since 2.0
class NOVTABLE ui_config_manager : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(ui_config_manager);
public:
	//! Registers a callback to receive notifications about colors/fonts change. \n
	//! Main thread only!
	virtual void add_callback(ui_config_callback*) = 0;
	//! Unregisters a callback to receive notifications about colors/fonts change. \n
	//! Main thread only!
	virtual void remove_callback(ui_config_callback*) = 0;

	//! Queries actual color to be used for the specified ui_color_* element. \n
	//! Main thread only! \n
	//! @returns True if color is user-overridden, false if system-default color should be used.
	virtual bool query_color(const GUID& p_what, t_ui_color& p_out) = 0;
	//! Queries font to be used for the specified ui_font_* element. \n
	//! Main thread only! \n
	//! The returned font handle is valid until the next font change callback cycle *completes*, that is, during a font change callback, both old and new handle are momentarily valid.
	virtual t_ui_font query_font(const GUID& p_what) = 0;	

	//! Helper using query_color(); returns true if dark mode is in effect, false otherwise.
	bool is_dark_mode();

#ifdef _WIN32
	t_ui_color getSysColor(int sysColorIndex);
#endif

	//! Special method that's safe to call without checking if ui_config_manager exists, that is, it works on foobar2000 < 2.0. \n
	//! Returns false if ui_conifg_manager doesn't exist and therefore dark mode isn't supported by this foobar2000 revision. \n
	//! Main thread only!
	static bool g_is_dark_mode();
};

//! \since 2.0
class NOVTABLE ui_config_manager_v2 : public ui_config_manager {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(ui_config_manager_v2, ui_config_manager)
public:
	//! Tells ui_config_manager about system theme having changed. \n
	//! Intended for keeping track of live dark mode toggle when foo_ui_std is not the active UI. Do not use.
	virtual void notify_system_theme_changed() = 0;
};

//! \since 2.0
//! Does nothing (fails to register quietly) if used in fb2k prior to 2.0 \n
//! Use in main thread only!
class ui_config_callback_impl : public ui_config_callback {
public:
	ui_config_callback_impl();
	~ui_config_callback_impl();
	ui_config_callback_impl(const ui_config_callback_impl&) = delete;
	void operator=(const ui_config_callback_impl&) = delete;
};
