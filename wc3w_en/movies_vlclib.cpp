/*
The MIT License (MIT)
Copyright � 2024 Matt Wells

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the �Software�), to deal in the
Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pch.h"

#include "movies_vlclib.h"
#include "configTools.h"
#include "wc3w.h"

//#include <iostream>
//#include <thread>
//#include <cstring>
//using namespace VLC;

/*const char* const vlc_options[] = {
    "--file-caching=300"//,
    //"--network-caching=150",
    //"--clock-jitter=0",
    //"--live-caching=150",
    //"--clock-synchro=0",
    //"-vvv",
    //"--drop-late-frames",
    //"--skip-frames"
     };*/
//const char* const vlc_options[] = { "--freetype-font=Incised901 Lt BT" };

//VLC::Instance vlc_instance = VLC::Instance(_countof(vlc_options), vlc_options);
VLC::Instance vlc_instance = VLC::Instance(0, nullptr);

std::string movie_dir;
std::string movie_ext;
std::string movie_config_path;

BOOL are_movie_path_setting_set = FALSE;
int branch_offset_ms = 0;

BOOL is_inflight_mono_shader_enabled = FALSE;
BOOL inflight_use_audio_from_file_if_present = 1;
SCALE_TYPE inflight_display_aspect_type = SCALE_TYPE::fit;

DWORD inflight_cockpit_bg_colour_argb = 0xFF000000;

LibVlc_MovieInflight* pMovie_vlc_Inflight = nullptr;

//____________________________________________________________________________________
static BOOL Create_Movie_Path_Inflight(const char* movie_name, std::string* p_retPath) {
    if (!p_retPath)
        return FALSE;

    p_retPath->clear();
    p_retPath->append(movie_dir);
    p_retPath->append("inflight\\");
    p_retPath->append(movie_name);
    p_retPath->append(movie_ext);


    Debug_Info("Create_Movie_Path_Inflight: %s", p_retPath->c_str());
    if (GetFileAttributesA(p_retPath->c_str()) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return TRUE;
}


//___________________________________________________________________________________________________________
static BOOL Create_Movie_Path_Inflight(const char* movie_name, DWORD appendix_offset, std::string* p_retPath) {
    if (!p_retPath)
        return FALSE;
    if (appendix_offset == -1)
        return Create_Movie_Path_Inflight(movie_name, p_retPath);
    if (appendix_offset >= 26) {
        Debug_Info("Create_Movie_Path_Inflight: appendix greater than 26: appendix:%d", appendix_offset);
        return FALSE;
    }
    char appendix[2] = { 'a'+ (char)appendix_offset , '\0'};

    p_retPath->clear();
    p_retPath->append(movie_dir);
    p_retPath->append("inflight\\");
    p_retPath->append(movie_name);
    p_retPath->append("_");
    p_retPath->append(appendix);
    p_retPath->append(movie_ext);


    Debug_Info("Create_Movie_Path_Inflight: %s", p_retPath->c_str());
    if (GetFileAttributesA(p_retPath->c_str()) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return TRUE;
}



//__________________________________________
static bool isPathRelative(const char* path) {

    if (isalpha(path[0])) {
        if (path[1] == ':' && path[2] == '\\')
            return false;
    }
    else if (path[0] == '\\')
        return false;
    return true;
}


//Set the absolute path for movie config file.
//_________________________________
static void Set_Movie_Config_Path() {
    movie_config_path.clear();
    if (isPathRelative(movie_dir.c_str())) {
        char* pMoviePath = new char[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, pMoviePath);
        movie_config_path.append(pMoviePath);
        movie_config_path.append("\\");
        delete[] pMoviePath;
    }
    movie_config_path.append(movie_dir);
    movie_config_path.append("movies.ini");
}


//______________________________
static BOOL Set_Movie_Settings() {
    if (are_movie_path_setting_set)
        return TRUE;
    
    char* char_buff = new char[MAX_PATH];
    char* char_ptr = char_buff;
  
    //set the hd movie directory.
    ConfigReadString("MOVIES", "PATH", CONFIG_MOVIES_PATH, char_buff, MAX_PATH);
    //skip over any space chars
    while (*char_ptr == ' ')
        char_ptr++;
    
    movie_dir = char_ptr;
    //make sure path ends with a back-slash.
    if (movie_dir.at(movie_dir.length() - 1) != '\\')
        movie_dir.append("\\");
    Debug_Info("movie directory: %s", movie_dir.c_str());

    Set_Movie_Config_Path();
    Debug_Info("movie_config_path: %s", movie_config_path.c_str());

    //get the hd movie extension.
    ConfigReadString("MOVIES", "EXT", CONFIG_MOVIES_EXT, char_buff, MAX_PATH);
    //skip over any space chars
    char_ptr = char_buff;
    while (*char_ptr == ' ')
        char_ptr++;
    
    movie_ext.clear();
    ////make sure extension begins with a dot.
    if (char_ptr[0] != '.')
        movie_ext.append(".");
    movie_ext.append(char_ptr);
    Debug_Info("movie extension: %s", movie_ext.c_str());

    //get the offset between movie branches in milliseconds.
    branch_offset_ms = ConfigReadInt("MOVIES", "BRANCH_OFFSET_MS", CONFIG_MOVIES_BRANCH_OFFSET_MS);
    Debug_Info("movie branch offset: %d ms", branch_offset_ms);


    inflight_use_audio_from_file_if_present = ConfigReadInt("MOVIES", "INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT", CONFIG_MOVIES_INFLIGHT_USE_AUDIO_FROM_FILE_IF_PRESENT);
    if(ConfigReadInt("MOVIES", "INFLIGHT_DISPLAY_ASPECT_TYPE", CONFIG_MOVIES_INFLIGHT_DISPLAY_ASPECT_TYPE) == 0)
        inflight_display_aspect_type = SCALE_TYPE::fill;
    else
        inflight_display_aspect_type = SCALE_TYPE::fit;

    inflight_cockpit_bg_colour_argb = 0xFF000000 | ConfigReadInt("MOVIES", "INFLIGHT_COCKPIT_BG_COLOUR_RGB", CONFIG_MOVIES_INFLIGHT_COCKPIT_BG_COLOUR_RGB);

    if (ConfigReadInt("MOVIES", "INFLIGHT_MONO_SHADER_ENABLE", CONFIG_INFLIGHT_MONO_SHADER_ENABLE)) {
        is_inflight_mono_shader_enabled = TRUE;
        DWORD colour = ConfigReadInt("MOVIES", "INFLIGHT_MONO_SHADER_COLOUR", CONFIG_INFLIGHT_MONO_SHADER_COLOUR);
        UINT brightness = ConfigReadInt("MOVIES", "INFLIGHT_MONO_SHADER_BRIGHTNESS", CONFIG_INFLIGHT_MONO_SHADER_BRIGHTNESS);
        UINT contrast = ConfigReadInt("MOVIES", "INFLIGHT_MONO_SHADER_CONTRAST", CONFIG_INFLIGHT_MONO_SHADER_CONTRAST);
        Inflight_Mono_Colour_Setup(colour, brightness, contrast);
    }

    delete[] char_buff;
    return are_movie_path_setting_set = TRUE;
}


//_________________________________________________________________________
BOOL Get_Movie_Name_From_Path(const char* mve_path, std::string* p_retPath) {
    
    if (!p_retPath)
        return FALSE;
    p_retPath->clear();

    char movie_name[9]{ 0 };

    const char* movie_name_start = strrchr(mve_path, '\\');

    if (movie_name_start) {
        movie_name_start += 1;
        strncpy_s(movie_name, _countof(movie_name), movie_name_start, _countof(movie_name) - 1);
        char* ext = strrchr(movie_name, '.');
        if (ext) {
            *ext = '\0';
        }
        char* c = movie_name;
        while (*c) {
            *c = tolower(*c);
            c++;
        }

        Debug_Info("movie name original: %s", movie_name);
        GetPrivateProfileStringA(movie_name, "name", movie_name, movie_name, _countof(movie_name), movie_config_path.c_str());
        //end string if a comment or space char is encountered.
        char* end_char = strchr(movie_name, ';');
        if (end_char)
            *end_char = '\0';
        end_char = strchr(movie_name, ' ');
        if (end_char)
            *end_char = '\0';

        Debug_Info("movie name fixed: %s", movie_name);
        p_retPath->append(movie_name);
    }
    Debug_Info("Get_Movie_Name_From_Path: %s", p_retPath->c_str());

    return TRUE;
}


//___________________________________________________________________________________________________
static BOOL Create_Movie_Path_Name_Branch(const char* movie_name, int branch, std::string* p_retPath) {
    if (!p_retPath)
        return FALSE;

    char c_branch[3] = "\0";
    if (branch < 26) {
        c_branch[0] = 'a' + branch;
        c_branch[1] = '\0';
    }
    else {
        branch -= 26;
        c_branch[0] = 'a' + branch;
        c_branch[1] = 'a' + branch;
        c_branch[2] = '\0';
    }
    Debug_Info("c_branch original: %s", c_branch);
    GetPrivateProfileStringA(movie_name, c_branch, c_branch, c_branch, _countof(c_branch), movie_config_path.c_str());
    //end string if a comment or space char is encountered.
    char* end_char = strchr(c_branch, ';');
    if (end_char)
        *end_char = '\0';
    end_char = strchr(c_branch, ' ');
    if (end_char)
        *end_char = '\0';

    Debug_Info("c_branch fixed: %s", c_branch);

    p_retPath->clear();

    p_retPath->append(movie_dir);
    p_retPath->append(movie_name);
    p_retPath->append(c_branch);
    p_retPath->append(movie_ext);


    Debug_Info("Create_Movie_Path: %s", p_retPath->c_str());
//    if (GetFileAttributesA(&p_retPath->c_str()[1]) == INVALID_FILE_ATTRIBUTES)
    if (GetFileAttributesA(p_retPath->c_str()) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return TRUE;
}


//_________________________________________________________________________________________
LibVlc_Movie::LibVlc_Movie(std::string movie_name, LONG* branch_list, LONG branch_list_num) {
    Debug_Info("LibVlc_Movie: Create Start");
    Set_Movie_Settings();
    next = nullptr;
    isPlaying = false;
    hasPlayed = false;
    isError = false;

    surface = nullptr;
    initialised_for_play = false;

    position = 0;
    paused = false;
    using namespace std::placeholders; // for `_1`

    mediaPlayer.eventManager().onPlaying(std::bind(&LibVlc_Movie::on_play, this));
    mediaPlayer.eventManager().onStopped(std::bind(&LibVlc_Movie::on_stopped, this));

    mediaPlayer.eventManager().onEncounteredError(std::bind(&LibVlc_Movie::on_encountered_error, this));
    mediaPlayer.eventManager().onTimeChanged(std::bind(&LibVlc_Movie::on_time_changed, this, _1));

    mediaPlayer.eventManager().onBuffering(std::bind(&LibVlc_Movie::on_buffering, this, _1));
    mediaPlayer.eventManager().onEndReached(std::bind(&LibVlc_Movie::on_end_reached, this));

    mediaPlayer.setVideoCallbacks(std::bind(&LibVlc_Movie::lock, this, _1), std::bind(&LibVlc_Movie::unlock, this, _1, _2), std::bind(&LibVlc_Movie::display, this, _1));
    mediaPlayer.setVideoFormatCallbacks(std::bind(&LibVlc_Movie::format, this, _1, _2, _3, _4, _5), std::bind(&LibVlc_Movie::cleanup, this));

    path.clear();

    list_num = branch_list_num;

    if (branch_list)
        branch = branch_list[list_num];
    else
        branch = -1;


    if (branch >= 0) {
        Debug_Info("LibVlc_Movie: add branch:%d", branch);
        if (!Create_Movie_Path_Name_Branch(movie_name.c_str(), branch, &path)) {
            Debug_Info("LibVlc_Movie: Create Movie Branch Path FAILED: %s, branch:%d", movie_name.c_str(), branch);
            isError = true;
        }
        else
            Debug_Info("LibVlc_Movie: Created Movie Branch Path: %s", path.c_str());

        //add next branch if present.
        branch_list_num++;
        if (branch_list[branch_list_num] >= 0)
            next = new LibVlc_Movie(movie_name, branch_list, branch_list_num);
    }
    else
        Debug_Info("LibVlc_Movie: Create Movie Branch NEG:%s, branch:%d", movie_name.c_str(), branch);

    Debug_Info("LibVlc_Movie: Create Done: %s", path.c_str());
}


//_______________________
bool LibVlc_Movie::Play() {
    if (isError)
        isPlaying = false;
    else if (initialised_for_play) {
        Debug_Info("LibVlc_Movie: Play: %s", path.c_str());
        mediaPlayer.setPause(false);
        if (next)
            next->InitialiseForPlay_Start();
        return isPlaying = true;
    }
    else {
        if (InitialiseForPlay_Start())
            return Play();

    }

    return isPlaying;
}


//___________________________________________________________
bool LibVlc_Movie::Play(LARGE_INTEGER play_end_time_in_ticks) {
    if (isError)
        isPlaying = false;
    else if (initialised_for_play) {
        Debug_Info("LibVlc_Movie: Play: %s", path.c_str());
        LARGE_INTEGER current_time_in_ticks;
        QueryPerformanceCounter(&current_time_in_ticks);
        if (next)
            next->InitialiseForPlay_Start();
        //offset play_end_time_in_ticks for for next movie
        //convert ms to ticks, -180ms seems about right.
        play_end_time_in_ticks.QuadPart += branch_offset_ms * p_wc3_frequency->QuadPart / 1000LL;
        //wait untill the right moment to start the next movie. 
        while (current_time_in_ticks.QuadPart < play_end_time_in_ticks.QuadPart) {
            Sleep(0);
            QueryPerformanceCounter(&current_time_in_ticks);
        }
        mediaPlayer.setPause(false);

        return isPlaying = true;
    }
    else {
        if (InitialiseForPlay_Start())
            return Play();

    }

    return isPlaying;
}


//___________________________________________
void LibVlc_Movie::SetMedia(std::string path) {
    VLC::Media media;

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    media = VLC::Media(path, VLC::Media::FromPath);
#else
    Debug_Info("LibVlc_Movie: SetMedia: %s", path.c_str());
    media = VLC::Media(vlc_instance, path, VLC::Media::FromPath);

#endif
    mediaPlayer.setMedia(media);

    //std::string path_sub = path;
   // size_t index = path_sub.find(movie_ext);
    //language_selected
    //0040A638 | > \833D 20974A00 CMP DWORD PTR DS : [4A9720] , 2

    //path_sub.replace(index, 5, ".fr.srt");
    //media.addSlave(VLC::MediaSlave::Type::Subtitle, 0, path_sub.c_str());


    //mediaPlayer.addSlave(VLC::MediaSlave::Type::Subtitle,  path.c_str(), false);

    //std::vector<VLC::MediaSlave> media_slaves = media.slaves();
    //for (int i = 0; i < media_slaves.size(); i++) {
    //    VLC::MediaSlave description = media_slaves[i];
    //    description.uri();
    //    Debug_Info("MediaSlave type: %d, uri: %s", description.type(), description.uri().c_str());
    //}

    //int subCount = mediaPlayer.spuCount();
    //Debug_Info("subCount: %d", subCount);

    initialised_for_play = false;
}

//_______________________________________________________
void LibVlc_Movie::on_time_changed(libvlc_time_t time_ms) {
    if (next && !next->IsPlaying()) {
        libvlc_time_t mediaLength = mediaPlayer.length();
        if (mediaLength > 0 && time_ms >= mediaLength - 1000) {
            Debug_Info("LibVlc_Movie: ON TIME CHANGED time_ms:%d:%d: dist:%d, %s", (LONG)(time_ms), (LONG)(mediaLength), (LONG)(mediaLength - time_ms), path.c_str());
            //Get the current number of ticks then add the number of tick till the end of the current movie.
            LARGE_INTEGER play_end_time_in_ticks;
            QueryPerformanceCounter(&play_end_time_in_ticks);
            play_end_time_in_ticks.QuadPart += (mediaLength - time_ms) * p_wc3_frequency->QuadPart / 1000LL;

            //Set the next movie up to play providing the time when the current movie should end.
            next->Play(play_end_time_in_ticks);

            LARGE_INTEGER play_init_end_time;
            QueryPerformanceCounter(&play_init_end_time);
            play_init_end_time.QuadPart -= play_end_time_in_ticks.QuadPart;
            play_init_end_time.QuadPart *= 1000LL;
            play_init_end_time.QuadPart /= p_wc3_frequency->QuadPart;
            Debug_Info("LibVlc_Movie: ON TIME CHANGED time taken: %d, %s", play_init_end_time.LowPart, path.c_str());
            isPlaying = false;
            hasPlayed = true;

        }
    }
}


//_______________________________________
void LibVlc_Movie::Initialise_Subtitles() {

    int subCount = mediaPlayer.spuCount();
    Debug_Info("LibVlc_Movie: subCount: %d", subCount);

    if (*p_wc3_subtitles_enabled == 0)
        mediaPlayer.setSpu(-1);
    else {
        Debug_Info("language ref: %d", *p_wc3_language_ref);
        std::string language;
        switch (*p_wc3_language_ref) {
        case 0:
            language = "[English]";
            break;
        case 1:
            language = "[German]";
            break;
        case 2:
            language = "[French]";
            break;
        default:
            return;
            break;
        }
        Debug_Info("LibVlc_Movie: language: %s", language.c_str());

        std::vector <VLC::TrackDescription> trackDescriptions = mediaPlayer.spuDescription();
        for (size_t i = 0; i < trackDescriptions.size(); i++) {
            VLC::TrackDescription description = trackDescriptions[i];
            //find a subtitle track description containing matching language text and set it for display.
            if (description.name().find(language) != std::string::npos) {
                Debug_Info("LibVlc_Movie: TrackDescription: %d, name: %s", description.id(), description.name().c_str());
                mediaPlayer.setSpu(description.id());
            }
        }
    }
}


//__________________________________________
bool LibVlc_Movie::InitialiseForPlay_Start() {
    if (initialised_for_play)
        return true;
    if (isPlaying) {
        Debug_Info("LibVlc_Movie: InitialiseForPlay_Start, Play still initialising - cut short: %s", path.c_str());
        InitialiseForPlay_End();
        return true;
    }
    Debug_Info("LibVlc_Movie: InitialiseForPlay_Start initialising...: %s", path.c_str());
    SetMedia(path);
    isPlaying = mediaPlayer.play();
    if (!isPlaying)
        isError = true;
    if (isError)
        return false;



    while (isPlaying)//play untill InitialiseForPlay_End called.
        Sleep(0);
    return true;
}


//________________________________________
void LibVlc_Movie::InitialiseForPlay_End() {

    Debug_Info("LibVlc_Movie: InitialiseForPlay_End: %s", path.c_str());
    if (!initialised_for_play) {
        initialised_for_play = true;
        Debug_Info("LibVlc_Movie: InitialiseForPlay_End - true: %s", path.c_str());
        mediaPlayer.setPause(true);
        isPlaying = false;
    }
}


//________________________________________________________________________________________________________________
LibVlc_MovieInflight::LibVlc_MovieInflight(const char* file_name, RECT* p_rc_dest_unscaled, DWORD appendix_offset) {
    //Debug_Info("LibVlc_MovieInflight: Create Start");
    Set_Movie_Settings();
    isPlaying = false;
    hasPlayed = false;
    isError = false;
    has_audio = false;

    surface = nullptr;
    surface_bg = nullptr;
    play_setup_start = false;
    play_setup_complete = false;

    position = 0;
    paused = false;
    rc_dest_unscaled = { 0,0,0,0 };

    char movie_name[16]{ 0 };

    strncpy_s(movie_name, _countof(movie_name), file_name, _countof(movie_name) - 1);
    char* ext = strrchr(movie_name, '.');
    if (ext) {
        *ext = '\0';
        //iff files modified to play movies divided into scenes DON'T INCLUDE an extension in their file name. 
        //if the movie file name has an extension, DON'T ADD a letter appendix by setting "appendix_offset = -1".
        appendix_offset = -1;
    }
    char* c = movie_name;
    while (*c) {
        *c = tolower(*c);
        c++;
    }

    if (p_rc_dest_unscaled) {
        CopyRect(&rc_dest_unscaled, p_rc_dest_unscaled);
        if (!surface_bg && inflight_display_aspect_type == SCALE_TYPE::fit) {
            //Debug_Info("surface_bg: %dx%d", rc_dest_unscaled.right - rc_dest_unscaled.left + 1, rc_dest_unscaled.bottom - rc_dest_unscaled.top + 1);
            surface_bg = new GEN_SURFACE(rc_dest_unscaled.right - rc_dest_unscaled.left + 1, rc_dest_unscaled.bottom - rc_dest_unscaled.top + 1, 32);
            //set backgound to roughly match cockpit monitor colour.
            surface_bg->Clear_Texture(inflight_cockpit_bg_colour_argb);

        }
    }
    else {
        Debug_Info("LibVlc_MovieInflight: destination Rect not set: %s", movie_name);
        isError = true;
    }

    using namespace std::placeholders; // for `_1`

    mediaPlayer.eventManager().onPlaying(std::bind(&LibVlc_MovieInflight::on_play, this));
    mediaPlayer.eventManager().onStopped(std::bind(&LibVlc_MovieInflight::on_stopped, this));

    mediaPlayer.eventManager().onEncounteredError(std::bind(&LibVlc_MovieInflight::on_encountered_error, this));
    mediaPlayer.eventManager().onTimeChanged(std::bind(&LibVlc_MovieInflight::on_time_changed, this, _1));

    mediaPlayer.eventManager().onBuffering(std::bind(&LibVlc_MovieInflight::on_buffering, this, _1));
    mediaPlayer.eventManager().onEndReached(std::bind(&LibVlc_MovieInflight::on_end_reached, this));


    mediaPlayer.setVideoCallbacks(std::bind(&LibVlc_MovieInflight::lock, this, _1), std::bind(&LibVlc_MovieInflight::unlock, this, _1, _2), std::bind(&LibVlc_MovieInflight::display, this, _1));
    mediaPlayer.setVideoFormatCallbacks(std::bind(&LibVlc_MovieInflight::format, this, _1, _2, _3, _4, _5), std::bind(&LibVlc_MovieInflight::cleanup, this));

    path.clear();

    if (!Create_Movie_Path_Inflight(movie_name, appendix_offset, &path)) {
        Debug_Info("LibVlc_MovieInflight: Create Movie Path FAILED: %s , %s", movie_name, path.c_str());
        isError = true;
    }
    else
        Debug_Info("LibVlc_MovieInflight: Created Movie Path: %s", path.c_str());

}


//______________________________________________
void LibVlc_MovieInflight::initialise_for_play() {
    // these need to be set once playback has started
    if (play_setup_start && !play_setup_complete) {
        //disable subtitles, we don't want subs overlayed on the inflight video.
        mediaPlayer.setSpu(-1);
        //setup audio
        if (!inflight_use_audio_from_file_if_present)
            mediaPlayer.setAudioTrack(-1);

        if (mediaPlayer.audioTrack() != -1) {
            Debug_Info("initialise_for_play: HD movie HAS AUDIO");
            has_audio = true;
        }
        //set the movie start time
        //mediaPlayer.setTime(time_ms_start);
        play_setup_complete = true;
    }

}


//_______________________________
bool LibVlc_MovieInflight::Play() {
    if (isError)
        isPlaying = false;
    else {
        //Debug_Info("LibVlc_MovieInflight: Play Start");
        SetMedia(path);
        isPlaying = mediaPlayer.play();
        if (!isPlaying)
            isError = true;
        if (isError)
            return false;
    }
    //Debug_Info("LibVlc_MovieInflight: Play END");
    return isPlaying;
}


//___________________________________________________
void LibVlc_MovieInflight::SetMedia(std::string path) {
    VLC::Media media;

#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    media = VLC::Media(path, VLC::Media::FromPath);
#else
    Debug_Info("LibVlc_Movie: SetMedia: %s", path.c_str());
    media = VLC::Media(vlc_instance, path, VLC::Media::FromPath);

#endif
    mediaPlayer.setMedia(media);

}


//___________________________________________________________________________
void LibVlc_MovieInflight::Update_Display_Dimensions(RECT* p_rc_gui_unscaled) {
    if (p_rc_gui_unscaled) {
        if (p_rc_gui_unscaled->left != rc_dest_unscaled.left || p_rc_gui_unscaled->right != rc_dest_unscaled.right || p_rc_gui_unscaled->top != rc_dest_unscaled.top || p_rc_gui_unscaled->bottom != rc_dest_unscaled.bottom) {
            CopyRect(&rc_dest_unscaled, p_rc_gui_unscaled);
            //Debug_Info("LibVlc_MovieInflight: Update_Display_Dimensions UPDATED: %s", path.c_str());
        }
        else
            return;
    }

    GEN_SURFACE* pSpace2D_surface = Get_Space2D_Surface();
    float scaleX = 1.0f, scaleY = 1.0f;
    float posX = 1.0f, posY = 1.0f;
    if (pSpace2D_surface) {
        pSpace2D_surface->GetScaledPixelDimensions(&scaleX, &scaleY);
        pSpace2D_surface->GetPosition(&posX, &posY);
    }

    float dest_width = (float)rc_dest_unscaled.right - rc_dest_unscaled.left + 1;
    float dest_height = (float)rc_dest_unscaled.bottom - rc_dest_unscaled.top + 1;
    
    unsigned int int_width = 0;
    unsigned int int_height = 0;
    mediaPlayer.size(0, &int_width, &int_height);

    if (inflight_display_aspect_type == SCALE_TYPE::fill) {
        if (surface) {
            surface->SetPosition(posX + rc_dest_unscaled.left * scaleX, posY + rc_dest_unscaled.top * scaleY);
            surface->SetScale(scaleX, scaleY);
            surface->SetScale(dest_width / (float)int_width * scaleX, dest_height / (float)int_height * scaleX);
        }
        return;
    }

    float movie_width = (float)int_width;
    float movie_height = (float)int_height;
    float movieRO = movie_width / movie_height;
    float destRO = dest_width / (float)dest_height;

    float x = 0;
    float y = 0;
    float width = dest_width;
    float height = dest_height;

    if (movieRO > destRO) {
        x = 0;
        width = (float)dest_width;
        height = (dest_width / movieRO);
        y = ((float)dest_height - height) / 2;
    }
    else {
        y = 0;
        height = (float)dest_height;
        width = dest_height * movieRO;
        x = ((float)dest_width - width) / 2;
    }

    if (surface) {
        surface->SetPosition(posX + (rc_dest_unscaled.left + x) * scaleX, posY + (rc_dest_unscaled.top + y) * scaleY);
        surface->SetScale(width / movie_width * scaleX, height / movie_height * scaleY);
    }
    if (surface_bg) {
        surface_bg->SetPosition(posX + rc_dest_unscaled.left * scaleX, posY + rc_dest_unscaled.top * scaleY);
        surface_bg->SetScale(scaleX, scaleY);
    }
}


//__________________________________
void LibVlc_MovieInflight::Display() {
    if (!play_setup_complete)//play_counter_started)
        return;
    if (surface_bg && *p_wc3_space_view_type == SPACE_VIEW_TYPE::Cockpit)
        surface_bg->Display();
    if (surface) {
        if (is_inflight_mono_shader_enabled)
            surface->Display(pd3d_PS_Greyscale_Tex_32);
        else
            surface->Display();
    }
}
