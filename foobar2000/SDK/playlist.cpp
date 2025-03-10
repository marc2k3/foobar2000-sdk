#include "foobar2000-sdk-pch.h"
#include "playlist.h"

namespace {
	class enum_items_callback_func : public playlist_manager::enum_items_callback {
	public:
		bool on_item(t_size p_index, const metadb_handle_ptr& p_location, bool b_selected) override { return f(p_index, p_location, b_selected); }
		playlist_manager::enum_items_func f;
	};
	class enum_items_callback_retrieve_item : public playlist_manager::enum_items_callback
	{
		metadb_handle_ptr m_item;
	public:
		enum_items_callback_retrieve_item() : m_item(0) {}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			(void)p_index; (void)b_selected;
			PFC_ASSERT(m_item.is_empty());
			m_item = p_location;
			return false;
		}
		inline const metadb_handle_ptr & get_item() {return m_item;}
	};

	class enum_items_callback_retrieve_selection : public playlist_manager::enum_items_callback
	{
		bool m_state;
	public:
		enum_items_callback_retrieve_selection() : m_state(false) {}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			(void)p_index; (void)p_location;
			m_state = b_selected;
			return false;
		}
		inline bool get_state() {return m_state;}
	};

	class enum_items_callback_retrieve_selection_mask : public playlist_manager::enum_items_callback
	{
		bit_array_var & m_out;
	public:
		enum_items_callback_retrieve_selection_mask(bit_array_var & p_out) : m_out(p_out) {}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			(void)p_location;
			m_out.set(p_index,b_selected);
			return true;
		}
	};

	class enum_items_callback_retrieve_all_items : public playlist_manager::enum_items_callback
	{
		pfc::list_base_t<metadb_handle_ptr> & m_out;
	public:
		enum_items_callback_retrieve_all_items(pfc::list_base_t<metadb_handle_ptr> & p_out) : m_out(p_out) {m_out.remove_all();}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			(void)p_index; (void)b_selected;
			m_out.add_item(p_location);
			return true;
		}
	};

	class enum_items_callback_retrieve_selected_items : public playlist_manager::enum_items_callback
	{
		pfc::list_base_t<metadb_handle_ptr> & m_out;
	public:
		enum_items_callback_retrieve_selected_items(pfc::list_base_t<metadb_handle_ptr> & p_out) : m_out(p_out) {m_out.remove_all();}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			(void)p_index;
			if (b_selected) m_out.add_item(p_location);
			return true;
		}
	};

	class enum_items_callback_count_selection : public playlist_manager::enum_items_callback
	{
		t_size m_counter,m_max;
	public:
		enum_items_callback_count_selection(t_size p_max) : m_max(p_max), m_counter(0) {}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected)
		{
			(void)p_index; (void)p_location;
			if (b_selected) 
			{
				if (++m_counter >= m_max) return false;
			}
			return true;
		}
		
		inline t_size get_count() {return m_counter;}
	};

}

void playlist_manager::playlist_get_all_items(t_size p_playlist,pfc::list_base_t<metadb_handle_ptr> & out)
{
	playlist_get_items(p_playlist,out, pfc::bit_array_true());
}

void playlist_manager::playlist_get_selected_items(t_size p_playlist,pfc::list_base_t<metadb_handle_ptr> & out)
{
	enum_items_callback_retrieve_selected_items cb(out);
	playlist_enum_items(p_playlist,cb,pfc::bit_array_true());
}

void playlist_manager::playlist_get_selection_mask(t_size p_playlist,bit_array_var & out)
{
	enum_items_callback_retrieve_selection_mask cb(out);
	playlist_enum_items(p_playlist,cb,pfc::bit_array_true());
}

bool playlist_manager::playlist_is_item_selected(t_size p_playlist,t_size p_item)
{
	enum_items_callback_retrieve_selection callback;
	playlist_enum_items(p_playlist,callback,pfc::bit_array_one(p_item));
	return callback.get_state();
}

metadb_handle_ptr playlist_manager::playlist_get_item_handle(t_size playlist, t_size item) {
	metadb_handle_ptr temp;
	if (!playlist_get_item_handle(temp, playlist, item)) throw pfc::exception_invalid_params();
	PFC_ASSERT( temp.is_valid() );
	return temp;

}
bool playlist_manager::playlist_get_item_handle(metadb_handle_ptr & p_out,t_size p_playlist,t_size p_item)
{
	enum_items_callback_retrieve_item callback;
	playlist_enum_items(p_playlist,callback,pfc::bit_array_one(p_item));
	p_out = callback.get_item();
	return p_out.is_valid();
}

void playlist_manager::g_make_selection_move_permutation(t_size * p_output,t_size p_count,const bit_array & p_selection,int p_delta) {
	pfc::create_move_items_permutation(p_output,p_count,p_selection,p_delta);
}

bool playlist_manager::playlist_move_selection(t_size p_playlist,int p_delta) {
	if (p_delta==0) return true;
	
	t_size count = playlist_get_item_count(p_playlist);
	
	pfc::array_t<t_size> order; order.set_size(count);
	pfc::array_t<bool> selection; selection.set_size(count);
	
	pfc::bit_array_var_table mask(selection.get_ptr(),selection.get_size());
	playlist_get_selection_mask(p_playlist, mask);
	g_make_selection_move_permutation(order.get_ptr(),count,mask,p_delta);
	return playlist_reorder_items(p_playlist,order.get_ptr(),count);
}

//retrieving status
t_size playlist_manager::activeplaylist_get_item_count()
{
	t_size playlist = get_active_playlist();
	if (playlist == SIZE_MAX) return 0;
	else return playlist_get_item_count(playlist);
}

void playlist_manager::playlist_enum_items(size_t which, enum_items_func f, const bit_array& mask) {
	enum_items_callback_func cb; cb.f = f;
	this->playlist_enum_items(which, cb, mask);
}

void playlist_manager::activeplaylist_enum_items(enum_items_callback & p_callback,const bit_array & p_mask)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_enum_items(playlist,p_callback,p_mask);
}
void playlist_manager::activeplaylist_enum_items(enum_items_func f, const bit_array& mask) {
	size_t playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_enum_items(playlist, f, mask);
}
t_size playlist_manager::activeplaylist_get_focus_item()
{
	t_size playlist = get_active_playlist();
	if (playlist == SIZE_MAX) return SIZE_MAX;
	else return playlist_get_focus_item(playlist);
}

bool playlist_manager::activeplaylist_get_name(pfc::string_base & p_out)
{
	t_size playlist = get_active_playlist();
	if (playlist == SIZE_MAX) return false;
	else return playlist_get_name(playlist,p_out);
}

//modifying playlist
bool playlist_manager::activeplaylist_reorder_items(const t_size * order,t_size count)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_reorder_items(playlist,order,count);
	else return false;
}

void playlist_manager::activeplaylist_set_selection(const bit_array & affected,const bit_array & status)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_set_selection(playlist,affected,status);
}

bool playlist_manager::activeplaylist_remove_items(const bit_array & mask)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_remove_items(playlist,mask);
	else return false;
}

bool playlist_manager::activeplaylist_replace_item(t_size p_item,const metadb_handle_ptr & p_new_item)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_replace_item(playlist,p_item,p_new_item);
	else return false;
}

void playlist_manager::activeplaylist_set_focus_item(t_size p_item)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_set_focus_item(playlist,p_item);
}

t_size playlist_manager::activeplaylist_insert_items(t_size p_base,const pfc::list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_insert_items(playlist,p_base,data,p_selection);
	else return SIZE_MAX;
}

void playlist_manager::activeplaylist_ensure_visible(t_size p_item)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_ensure_visible(playlist,p_item);
}

bool playlist_manager::activeplaylist_rename(const char * p_name,t_size p_name_len)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_rename(playlist,p_name,p_name_len);
	else return false;
}

bool playlist_manager::activeplaylist_is_item_selected(t_size p_item)
{
	t_size playlist = get_active_playlist();
	if (playlist != pfc_infinite) return playlist_is_item_selected(playlist,p_item);
	else return false;
}

metadb_handle_ptr playlist_manager::activeplaylist_get_item_handle(t_size p_item) {
	metadb_handle_ptr temp;
	if (!activeplaylist_get_item_handle(temp, p_item)) throw pfc::exception_invalid_params();
	PFC_ASSERT( temp.is_valid() );
	return temp;
}
bool playlist_manager::activeplaylist_get_item_handle(metadb_handle_ptr & p_out,t_size p_item)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_get_item_handle(p_out,playlist,p_item);
	else return false;
}

void playlist_manager::activeplaylist_move_selection(int p_delta)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_move_selection(playlist,p_delta);
}

void playlist_manager::activeplaylist_get_selection_mask(bit_array_var & out)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_get_selection_mask(playlist,out);
}

void playlist_manager::activeplaylist_get_all_items(pfc::list_base_t<metadb_handle_ptr> & out)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_get_all_items(playlist,out);
}

void playlist_manager::activeplaylist_get_selected_items(pfc::list_base_t<metadb_handle_ptr> & out)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_get_selected_items(playlist,out);
}

bool playlist_manager::remove_playlist(t_size idx)
{
	return remove_playlists(pfc::bit_array_one(idx));
}

bool playlist_incoming_item_filter::process_location(const char * url,pfc::list_base_t<metadb_handle_ptr> & out,bool filter,const char * p_mask,const char * p_exclude,fb2k::hwnd_t p_parentwnd)
{
	return process_locations(pfc::list_single_ref_t<const char*>(url),out,filter,p_mask,p_exclude,p_parentwnd);
}

void playlist_manager::playlist_clear(t_size p_playlist)
{
	playlist_remove_items(p_playlist, pfc::bit_array_true());
}

void playlist_manager::activeplaylist_clear()
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_clear(playlist);
}

bool playlist_manager::playlist_update_content(t_size playlist, metadb_handle_list_cref content, bool bUndoBackup) {
	metadb_handle_list old;
	playlist_get_all_items(playlist, old);
	if (old.get_size() == 0) {
		if (content.get_size() == 0) return false;
		if (bUndoBackup) playlist_undo_backup(playlist);
		playlist_add_items(playlist, content, pfc::bit_array_false());
		return true;
	}
	pfc::avltree_t<metadb_handle::nnptr> itemsOld, itemsNew;

	for(t_size walk = 0; walk < old.get_size(); ++walk) itemsOld += old[walk];
	for(t_size walk = 0; walk < content.get_size(); ++walk) itemsNew += content[walk];
	pfc::bit_array_bittable removeMask(old.get_size());
	pfc::bit_array_bittable filterMask(content.get_size());
	bool gotNew = false, filterNew = false, gotRemove = false;
	for(t_size walk = 0; walk < content.get_size(); ++walk) {
		const bool state = !itemsOld.have_item(content[walk]);
		if (state) gotNew = true;
		else filterNew = true;
		filterMask.set(walk, state);
	}
	for(t_size walk = 0; walk < old.get_size(); ++walk) {
		const bool state = !itemsNew.have_item(old[walk]);
		if (state) gotRemove = true;
		removeMask.set(walk, state);
	}
	if (!gotNew && !gotRemove) return false;
	if (bUndoBackup) playlist_undo_backup(playlist);
	if (gotRemove) {
		playlist_remove_items(playlist, removeMask);
	}
	if (gotNew) {
		if (filterNew) {
			metadb_handle_list temp(content);
			temp.filter_mask(filterMask);
			playlist_add_items(playlist, temp, pfc::bit_array_false());
		} else {
			playlist_add_items(playlist, content, pfc::bit_array_false());
		}
	}

	{
		playlist_get_all_items(playlist, old);
		pfc::array_t<t_size> order;
		if (pfc::guess_reorder_pattern<pfc::list_base_const_t<metadb_handle_ptr> >(order, old, content)) {
			playlist_reorder_items(playlist, order.get_ptr(), order.get_size());
		}
	}
	return true;
}
bool playlist_manager::playlist_add_items(t_size playlist,const pfc::list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection)
{
	return playlist_insert_items(playlist, SIZE_MAX, data, p_selection) != SIZE_MAX;
}

bool playlist_manager::activeplaylist_add_items(const pfc::list_base_const_t<metadb_handle_ptr> & data,const bit_array & p_selection)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_add_items(playlist,data,p_selection);
	else return false;
}

bool playlist_manager::playlist_insert_items_filter(t_size p_playlist,t_size p_base,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,bool p_select)
{
	metadb_handle_list temp;
	if (!playlist_incoming_item_filter::get()->filter_items(p_data,temp))
		return false;
	return playlist_insert_items(p_playlist,p_base,temp, pfc::bit_array_val(p_select)) != SIZE_MAX;
}

bool playlist_manager::activeplaylist_insert_items_filter(t_size p_base,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,bool p_select)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_insert_items_filter(playlist,p_base,p_data,p_select);
	else return false;
}

bool playlist_manager::playlist_insert_locations(t_size p_playlist,t_size p_base,const pfc::list_base_const_t<const char*> & p_urls,bool p_select,fb2k::hwnd_t p_parentwnd)
{
	metadb_handle_list temp;
	if (!playlist_incoming_item_filter::get()->process_locations(p_urls,temp,true,0,0,p_parentwnd)) return false;
	return playlist_insert_items(p_playlist,p_base,temp, pfc::bit_array_val(p_select)) != SIZE_MAX;
}

bool playlist_manager::activeplaylist_insert_locations(t_size p_base,const pfc::list_base_const_t<const char*> & p_urls,bool p_select,fb2k::hwnd_t p_parentwnd)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_insert_locations(playlist,p_base,p_urls,p_select,p_parentwnd);
	else return false;
}

bool playlist_manager::playlist_add_items_filter(t_size p_playlist,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,bool p_select)
{
	return playlist_insert_items_filter(p_playlist,SIZE_MAX,p_data,p_select);
}

bool playlist_manager::activeplaylist_add_items_filter(const pfc::list_base_const_t<metadb_handle_ptr> & p_data,bool p_select)
{
	return activeplaylist_insert_items_filter(SIZE_MAX,p_data,p_select);
}

bool playlist_manager::playlist_add_locations(t_size p_playlist,const pfc::list_base_const_t<const char*> & p_urls,bool p_select,fb2k::hwnd_t p_parentwnd)
{
	return playlist_insert_locations(p_playlist,SIZE_MAX,p_urls,p_select,p_parentwnd);
}
bool playlist_manager::activeplaylist_add_locations(const pfc::list_base_const_t<const char*> & p_urls,bool p_select,fb2k::hwnd_t p_parentwnd)
{
	return activeplaylist_insert_locations(SIZE_MAX,p_urls,p_select,p_parentwnd);
}

void playlist_manager::reset_playing_playlist()
{
	set_playing_playlist(get_active_playlist());
}

void playlist_manager::playlist_clear_selection(t_size p_playlist)
{
	playlist_set_selection(p_playlist, pfc::bit_array_true(), pfc::bit_array_false());
}

void playlist_manager::activeplaylist_clear_selection()
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_clear_selection(playlist);
}

void playlist_manager::activeplaylist_undo_backup()
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_undo_backup(playlist);
}

bool playlist_manager::activeplaylist_undo_restore()
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_undo_restore(playlist);
	else return false;
}

bool playlist_manager::activeplaylist_redo_restore()
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_redo_restore(playlist);
	else return false;
}

void playlist_manager::playlist_remove_selection(t_size p_playlist,bool p_crop)
{
	pfc::bit_array_bittable table(playlist_get_item_count(p_playlist));
	playlist_get_selection_mask(p_playlist,table);
	if (p_crop) playlist_remove_items(p_playlist, pfc::bit_array_not(table));
	else playlist_remove_items(p_playlist,table);
}

void playlist_manager::activeplaylist_remove_selection(bool p_crop)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_remove_selection(playlist,p_crop);
}

void playlist_manager::activeplaylist_item_format_title(t_size p_item,titleformat_hook * p_hook,pfc::string_base & out,const service_ptr_t<titleformat_object> & p_script,titleformat_text_filter * p_filter,play_control::t_display_level p_playback_info_level)
{
	t_size playlist = get_active_playlist();
	if (playlist == SIZE_MAX) out = "NJET";
	else playlist_item_format_title(playlist,p_item,p_hook,out,p_script,p_filter,p_playback_info_level);
}

void playlist_manager::playlist_set_selection_single(t_size p_playlist,t_size p_item,bool p_state)
{
	playlist_set_selection(p_playlist, pfc::bit_array_one(p_item), pfc::bit_array_val(p_state));
}

void playlist_manager::activeplaylist_set_selection_single(t_size p_item,bool p_state)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) playlist_set_selection_single(playlist,p_item,p_state);
}

t_size playlist_manager::playlist_get_selection_count(t_size p_playlist,t_size p_max)
{
	enum_items_callback_count_selection callback(p_max);
	playlist_enum_items(p_playlist,callback, pfc::bit_array_true());
	return callback.get_count();
}

t_size playlist_manager::activeplaylist_get_selection_count(t_size p_max)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_get_selection_count(playlist,p_max);
	else return 0;
}

bool playlist_manager::playlist_get_focus_item_handle(metadb_handle_ptr & p_out,t_size p_playlist)
{
	t_size index = playlist_get_focus_item(p_playlist);
	if (index == SIZE_MAX) return false;
	return playlist_get_item_handle(p_out,p_playlist,index);
}

bool playlist_manager::activeplaylist_get_focus_item_handle(metadb_handle_ptr & p_out)
{
	t_size playlist = get_active_playlist();
	if (playlist != SIZE_MAX) return playlist_get_focus_item_handle(p_out,playlist);
	else return false;
}

t_size playlist_manager::find_playlist(const char * p_name,t_size p_name_length)
{
	t_size n, m = get_playlist_count();
	pfc::string_formatter temp;
	for(n=0;n<m;n++) {
		if (!playlist_get_name(n,temp)) break;
		if (stricmp_utf8_ex(temp,temp.length(),p_name,p_name_length) == 0) return n;
	}
	return SIZE_MAX;
}

t_size playlist_manager::find_or_create_playlist_unlocked(const char * p_name, t_size p_name_length) {
	t_size n, m = get_playlist_count();
	pfc::string_formatter temp;
	for(n=0;n<m;n++) {
		if (!playlist_lock_is_present(n) && playlist_get_name(n,temp)) {
			if (stricmp_utf8_ex(temp,SIZE_MAX,p_name,p_name_length) == 0) return n;
		}
	}
	return create_playlist(p_name,p_name_length, SIZE_MAX);
}
t_size playlist_manager::find_or_create_playlist(const char * p_name,t_size p_name_length)
{
	t_size index = find_playlist(p_name,p_name_length);
	if (index != SIZE_MAX) return index;
	return create_playlist(p_name,p_name_length, SIZE_MAX);
}

t_size playlist_manager::create_playlist_autoname(t_size p_index) {	
	static const char new_playlist_text[] = "New Playlist";
	if (find_playlist(new_playlist_text, SIZE_MAX) == SIZE_MAX) return create_playlist(new_playlist_text,SIZE_MAX,p_index);
	for(t_size walk = 2; ; walk++) {
		pfc::string_fixed_t<64> namebuffer;
		namebuffer << new_playlist_text << " (" << walk << ")";
		if (find_playlist(namebuffer, SIZE_MAX) == SIZE_MAX) return create_playlist(namebuffer,SIZE_MAX,p_index);
	}
}

bool playlist_manager::activeplaylist_sort_by_format(const char * spec,bool p_sel_only)
{
	t_size playlist = get_active_playlist();
	if (playlist != pfc_infinite) return playlist_sort_by_format(playlist,spec,p_sel_only);
	else return false;
}

bool playlist_manager::highlight_playing_item()
{
	t_size playlist,item;
	if (!get_playing_item_location(&playlist,&item)) return false;
	set_active_playlist(playlist);
	playlist_set_focus_item(playlist,item);
	playlist_set_selection(playlist, pfc::bit_array_true(), pfc::bit_array_one(item));
	playlist_ensure_visible(playlist,item);
	return true;
}

void playlist_manager::playlist_get_items(t_size p_playlist,pfc::list_base_t<metadb_handle_ptr> & out,const bit_array & p_mask)
{
	enum_items_callback_retrieve_all_items cb(out);
	playlist_enum_items(p_playlist,cb,p_mask);
}

void playlist_manager::activeplaylist_get_items(pfc::list_base_t<metadb_handle_ptr> & out,const bit_array & p_mask)
{
	t_size playlist = get_active_playlist();
	if (playlist != pfc_infinite) playlist_get_items(playlist,out,p_mask);
	else out.remove_all();
}

void playlist_manager::active_playlist_fix()
{
	t_size playlist = get_active_playlist();
	if (playlist == pfc_infinite)
	{
		t_size max = get_playlist_count();
		if (max == 0)
		{
			create_playlist_autoname();
		}
		set_active_playlist(0);
	}
}

namespace {
	class enum_items_callback_remove_list : public playlist_manager::enum_items_callback
	{
		const metadb_handle_list & m_data;
		bit_array_var & m_table;
		t_size m_found;
	public:
		enum_items_callback_remove_list(const metadb_handle_list & p_data,bit_array_var & p_table) : m_data(p_data), m_table(p_table), m_found(0) {}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected) override
		{
			(void)b_selected;
			bool found = m_data.bsearch_by_pointer(p_location) != pfc_infinite;
			m_table.set(p_index,found);
			if (found) m_found++;
			return true;
		}
		
		inline t_size get_found() const {return m_found;}
	};
}

void playlist_manager::remove_items_from_all_playlists(const pfc::list_base_const_t<metadb_handle_ptr> & p_data)
{
	t_size playlist_num, playlist_max = get_playlist_count();
	if (playlist_max != pfc_infinite)
	{
		metadb_handle_list temp;
		temp.add_items(p_data);
		temp.sort_by_pointer();
		for(playlist_num = 0; playlist_num < playlist_max; playlist_num++ )
		{
			t_size playlist_item_count = playlist_get_item_count(playlist_num);
			if (playlist_item_count == pfc_infinite) break;
			pfc::bit_array_bittable table(playlist_item_count);
			enum_items_callback_remove_list callback(temp,table);
			playlist_enum_items(playlist_num,callback, pfc::bit_array_true());
			if (callback.get_found()>0)
				playlist_remove_items(playlist_num,table);
		}
	}
}

bool playlist_manager::get_all_items(pfc::list_base_t<metadb_handle_ptr> & out)
{
	t_size n, m = get_playlist_count();
	if (m == pfc_infinite) return false;
	enum_items_callback_retrieve_all_items callback(out);
	for(n=0;n<m;n++)
	{
		playlist_enum_items(n,callback,pfc::bit_array_true());
	}
	return true;
}

t_uint32 playlist_manager::activeplaylist_lock_get_filter_mask()
{
	t_size playlist = get_active_playlist();
	if (playlist == SIZE_MAX) return UINT32_MAX;
	else return playlist_lock_get_filter_mask(playlist);
}

bool playlist_manager::activeplaylist_is_undo_available()
{
	t_size playlist = get_active_playlist();
	if (playlist == pfc_infinite) return false;
	else return playlist_is_undo_available(playlist);
}

bool playlist_manager::activeplaylist_is_redo_available()
{
	t_size playlist = get_active_playlist();
	if (playlist == pfc_infinite) return false;
	else return playlist_is_redo_available(playlist);
}

bool playlist_manager::remove_playlist_user() {
	size_t a = this->get_active_playlist();
	if (a == SIZE_MAX) {
		// FIX ME implement toast
#ifdef _WIN32
		MessageBeep(0);
#endif
		return false;
	}
	return this->remove_playlist_user(a);
}

bool playlist_manager::remove_playlist_user(size_t which) {
	if (this->get_playlist_count() == 1) {
		// FIX ME implement toast
#ifdef _WIN32
		MessageBeep(0);
#endif
		return false;
	}
	
	if (!this->remove_playlist_switch(which)) {
		// FIX ME implement toast
#ifdef _WIN32
		MessageBeep(0);
#endif
		return false;
	}
	return true;
}

bool playlist_manager::remove_playlist_switch(t_size idx)
{
	bool need_switch = get_active_playlist() == idx;
	if (remove_playlist(idx))
	{
		if (need_switch)
		{
			t_size total = get_playlist_count();
			if (total > 0)
			{
				if (idx >= total) idx = total-1;
				set_active_playlist(idx);
			}
		}
		return true;
	}
	else return false;
}



bool t_playback_queue_item::operator==(const t_playback_queue_item & p_item) const
{
	return m_handle == p_item.m_handle && m_playlist == p_item.m_playlist && m_item == p_item.m_item;
}

bool t_playback_queue_item::operator!=(const t_playback_queue_item & p_item) const
{
	return m_handle != p_item.m_handle || m_playlist != p_item.m_playlist || m_item != p_item.m_item;
}



bool playlist_manager::activeplaylist_execute_default_action(t_size p_item) {
	t_size idx = get_active_playlist();
	if (idx == pfc_infinite) return false;
	else return playlist_execute_default_action(idx,p_item);
}

namespace {
	class completion_notify_dfd : public completion_notify {
	public:
		completion_notify_dfd(const pfc::list_base_const_t<metadb_handle_ptr> & p_data,service_ptr_t<process_locations_notify> p_notify) : m_data(p_data), m_notify(p_notify) {}
		void on_completion(unsigned p_code) {
			switch(p_code) {
			case metadb_io::load_info_aborted:
				m_notify->on_aborted();
				break;
			default:
				m_notify->on_completion(m_data);
				break;
			}
		}
	private:
		metadb_handle_list m_data;
		service_ptr_t<process_locations_notify> m_notify;
	};
};

void dropped_files_data_impl::to_handles_async_ex(t_uint32 p_op_flags,fb2k::hwnd_t p_parentwnd,service_ptr_t<process_locations_notify> p_notify) {
	if (m_is_paths) {
		playlist_incoming_item_filter_v2::get()->process_locations_async(
			m_paths,
			p_op_flags,
			NULL,
			NULL,
			p_parentwnd,
			p_notify);
	} else {
		t_uint32 flags = 0;
		if (p_op_flags & playlist_incoming_item_filter_v2::op_flag_background) flags |= metadb_io_v2::op_flag_background;
		if (p_op_flags & playlist_incoming_item_filter_v2::op_flag_delay_ui) flags |= metadb_io_v2::op_flag_delay_ui;
		metadb_io_v2::get()->load_info_async(m_handles,metadb_io::load_info_default,p_parentwnd,flags,new service_impl_t<completion_notify_dfd>(m_handles,p_notify));
	}
}
void dropped_files_data_impl::to_handles_async(bool p_filter,fb2k::hwnd_t p_parentwnd,service_ptr_t<process_locations_notify> p_notify) {
	to_handles_async_ex(p_filter ? 0 : playlist_incoming_item_filter_v2::op_flag_no_filter,p_parentwnd,p_notify);
}

bool dropped_files_data_impl::to_handles(pfc::list_base_t<metadb_handle_ptr> & p_out,bool p_filter,fb2k::hwnd_t p_parentwnd) {
	if (m_is_paths) {
		return playlist_incoming_item_filter::get()->process_locations(m_paths,p_out,p_filter,NULL,NULL,p_parentwnd);
	} else {
		if (metadb_io::get()->load_info_multi(m_handles,metadb_io::load_info_default,p_parentwnd,true) == metadb_io::load_info_aborted) return false;
		p_out = m_handles;
		return true;
	}
}

void playlist_manager::playlist_activate_delta(int p_delta) {
	const t_size total = get_playlist_count();
	if (total > 0) {
		t_size active = get_active_playlist();
		
		//clip p_delta to -(total-1)...(total-1) range
		if (p_delta < 0) {
			p_delta = - ( (-p_delta) % (t_ssize)total );
		} else {
			p_delta = p_delta % total;
		}
		if (p_delta != 0) {
			if (active == pfc_infinite) {
				//special case when no playlist is active
				if (p_delta > 0) {
					active = (t_size)(p_delta - 1);
				} else {
					active = (total + p_delta);//p_delta is negative
				}
			} else {
				active = (t_size) (active + total + p_delta) % total;
			}
			set_active_playlist(active % total);
		}
	}
}
namespace {
	class enum_items_callback_get_selected_count : public playlist_manager::enum_items_callback {
	public:
		enum_items_callback_get_selected_count() : m_found() {}
		t_size get_count() const {return m_found;}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected) override {
			(void)p_index; (void)p_location;
			if (b_selected) m_found++;
			return true;
		}
	private:
		t_size m_found;
	};
}
t_size playlist_manager::playlist_get_selected_count(t_size p_playlist,bit_array const & p_mask) {
	enum_items_callback_get_selected_count callback;
	playlist_enum_items(p_playlist,callback,p_mask);
	return callback.get_count();
}

namespace {
	class enum_items_callback_find_item : public playlist_manager::enum_items_callback {
	public:
		enum_items_callback_find_item(metadb_handle_ptr p_lookingFor) : m_lookingFor(p_lookingFor) {}
		t_size result() const {return m_result;}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected) override {
			(void)b_selected;
			if (p_location == m_lookingFor) {
				m_result = p_index;
				return false;
			} else {
				return true;
			}
		}
	private:
		metadb_handle_ptr m_lookingFor;
		size_t m_result = SIZE_MAX;
	};
	class enum_items_callback_find_item_selected : public playlist_manager::enum_items_callback {
	public:
		enum_items_callback_find_item_selected(metadb_handle_ptr p_lookingFor) : m_lookingFor(p_lookingFor) {}
		t_size result() const {return m_result;}
		bool on_item(t_size p_index,const metadb_handle_ptr & p_location,bool b_selected) {
			if (b_selected && p_location == m_lookingFor) {
				m_result = p_index;
				return false;
			} else {
				return true;
			}
		}
	private:
		metadb_handle_ptr m_lookingFor;
		size_t m_result = SIZE_MAX;
	};
}

bool playlist_manager::playlist_find_item(t_size p_playlist,metadb_handle_ptr p_item,t_size & p_result) {
	enum_items_callback_find_item callback(p_item);
	playlist_enum_items(p_playlist,callback,pfc::bit_array_true());
	t_size result = callback.result();
	if (result == SIZE_MAX) return false;
	p_result = result;
	return true;
}
bool playlist_manager::playlist_find_item_selected(t_size p_playlist,metadb_handle_ptr p_item,t_size & p_result) {
	enum_items_callback_find_item_selected callback(p_item);
	playlist_enum_items(p_playlist,callback,pfc::bit_array_true());
	t_size result = callback.result();
	if (result == SIZE_MAX) return false;
	p_result = result;
	return true;
}
t_size playlist_manager::playlist_set_focus_by_handle(t_size p_playlist,metadb_handle_ptr p_item) {
	t_size index;
	if (!playlist_find_item(p_playlist,p_item,index)) index = SIZE_MAX;
	playlist_set_focus_item(p_playlist,index);
	return index;
}
bool playlist_manager::activeplaylist_find_item(metadb_handle_ptr p_item,t_size & p_result) {
	t_size playlist = get_active_playlist();
	if (playlist == SIZE_MAX) return false;
	return playlist_find_item(playlist,p_item,p_result);
}
t_size playlist_manager::activeplaylist_set_focus_by_handle(metadb_handle_ptr p_item) {
	t_size playlist = get_active_playlist();
	if (playlist == SIZE_MAX) return SIZE_MAX;
	return playlist_set_focus_by_handle(playlist,p_item);
}

#ifdef _WIN32
pfc::com_ptr_t<interface IDataObject> playlist_incoming_item_filter::create_dataobject_ex(metadb_handle_list_cref data) {
	pfc::com_ptr_t<interface IDataObject> temp; temp.attach( create_dataobject(data) ); PFC_ASSERT( temp.is_valid() ); return temp;
}
#endif

void playlist_manager_v3::recycler_restore_by_id(t_uint32 id) {
	t_size which = recycler_find_by_id(id);
	if (which != ~0) recycler_restore(which);
}

t_size playlist_manager_v3::recycler_find_by_id(t_uint32 id) {
	const t_size total = recycler_get_count();
	for(t_size walk = 0; walk < total; ++walk) {
		if (id == recycler_get_id(walk)) return walk;
	}
	return SIZE_MAX;
}



typedef pfc::map_t< const char*, metadb_handle_list, metadb::path_comparator > byPath_t;
static void rechapter_worker(playlist_manager* api, byPath_t const& byPath) {
	if (byPath.get_count() == 0) return;
	const size_t numPlaylists = api->get_playlist_count();
	for (size_t walkPlaylist = 0; walkPlaylist < numPlaylists; ++walkPlaylist) {
		if (!api->playlist_lock_is_present(walkPlaylist)) {
			auto itemCount = [=] {
				return api->playlist_get_item_count(walkPlaylist);
			};
			auto itemHandle = [=](size_t item) -> metadb_handle_ptr {
				return api->playlist_get_item_handle(walkPlaylist, item);
			};

			for (size_t walkItem = 0; walkItem < itemCount(); ) {
				auto item = itemHandle(walkItem);
				auto itemPath = item->get_path();
				auto match = byPath.find(itemPath);
				if (match.is_valid() ) {
					pfc::avltree_t<uint32_t> subsongs;
					auto base = walkItem;
					bool bSel = false;
					for (++walkItem; walkItem < itemCount(); ++walkItem) {
						auto handle = itemHandle(walkItem);
						if (metadb::path_compare(itemPath, handle->get_path()) != 0) break;
						if (!subsongs.add_item_check(handle->get_subsong_index())) break;

						bSel = bSel || api->playlist_is_item_selected(walkPlaylist, walkItem);
					}

					const auto& newItems = match->m_value;
					// REMOVE base ... walkItem range and insert newHandles at base
					api->playlist_remove_items(walkPlaylist, pfc::bit_array_range(base, walkItem - base));
					api->playlist_insert_items(walkPlaylist, base, newItems, pfc::bit_array_val(bSel));
					walkItem = base + newItems.get_size();
				}
				else {
					++walkItem;
				}
			}
		}
	}
}

void playlist_manager::on_files_rechaptered( metadb_handle_list_cref newHandles ) {
	pfc::map_t< const char*, metadb_handle_list, metadb::path_comparator > byPath;

	const size_t total = newHandles.get_count();
	for( size_t w = 0; w < total; ++w ) {
		auto handle = newHandles[w];
		byPath[ handle->get_path() ] += handle;
	}

	rechapter_worker(this, byPath);
}

void playlist_manager::on_file_rechaptered(const char* path, metadb_handle_list_cref newItems) {
	// obsolete method
	(void)path;
	on_files_rechaptered(newItems);
}

namespace {
	class process_locations_notify_lambda : public process_locations_notify {
	public:
		process_locations_notify::func_t f;
		void on_completion(metadb_handle_list_cref p_items) override {
			PFC_ASSERT(f != nullptr);
			f(p_items);
		}
		void on_aborted() override {}
	};
}
process_locations_notify::ptr process_locations_notify::create(func_t arg) {
	PFC_ASSERT(arg != nullptr);
	auto ret = fb2k::service_new< process_locations_notify_lambda >();
	ret->f = arg;
	return ret;
}