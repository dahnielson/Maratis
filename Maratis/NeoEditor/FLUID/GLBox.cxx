#include "GLBox.h"
#include "../MWindow/MWindow.h"
#include <MEngine.h>
#include <MLoaders/MImageLoader.h>
#include <MLog.h>

#include <MMouse.h>
#include <MGui.h>
#include "../MFilesUpdate/MFilesUpdate.h"
#include "../Maratis/Maratis.h"
#include "Callbacks.h"
#include <MCore.h>
#include "../MRenderArray/MRenderArray.h"
#include "MainWindow.h"

#include <FL/Enumerations.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Double_Window.H>

#include "../MPluginScript/MPluginScript.h"

#include <algorithm>
#include <string>

extern Fl_Double_Window* main_window;
extern EditorWindow window;

extern void update_scene_tree();

void update_editor(void*)
{
    if(!window.glbox->maratis_init)
        return;

    MGame* game = MEngine::getInstance()->getGame();
    MInputContext* input = MEngine::getInstance()->getInputContext();

    if(window.inputMethod == NULL && !game->isRunning())
    {
        if(input->isKeyPressed("W"))
        {
            int direction = -1;

            MOCamera * vue = Maratis::getInstance()->getPerspectiveVue();

            if(vue->isOrtho())
            {
                vue->setFov(vue->getFov() + direction*translation_speed);
            }


            vue->setPosition(vue->getPosition() + vue->getRotatedVector(MVector3(0,0,direction*translation_speed)));
            vue->updateMatrix();

            //window.glbox->redraw();
        }
        else if(input->isKeyPressed("S"))
        {
            int direction = 1;

            MOCamera * vue = Maratis::getInstance()->getPerspectiveVue();
            if(vue->isOrtho())
            {
                vue->setFov(vue->getFov() + direction*translation_speed);
            }

            vue->setPosition(vue->getPosition() + vue->getRotatedVector(MVector3(0,0,direction*translation_speed)));
            vue->updateMatrix();

            //window.glbox->redraw();
        }

        if(input->isKeyPressed("A"))
        {
            int direction = -1;

            MOCamera * vue = Maratis::getInstance()->getPerspectiveVue();
            vue->setPosition(vue->getPosition() + vue->getRotatedVector(MVector3(direction*translation_speed,0,0)));
            vue->updateMatrix();

            //window.glbox->redraw();
        }
        else if(input->isKeyPressed("D"))
        {
            int direction = 1;

            MOCamera * vue = Maratis::getInstance()->getPerspectiveVue();
            vue->setPosition(vue->getPosition() + vue->getRotatedVector(MVector3(direction*translation_speed,0,0)));
            vue->updateMatrix();

            //window.glbox->redraw();
        }

        if(input->isKeyPressed("E"))
        {
            MOCamera * vue = Maratis::getInstance()->getPerspectiveVue();
            vue->setPosition(vue->getPosition()+vue->getRotatedVector(MVector3(0,translation_speed, 0)));
            vue->updateMatrix();

            //window.glbox->redraw();
        }
        else if(input->isKeyPressed("C"))
        {
            MOCamera * vue = Maratis::getInstance()->getPerspectiveVue();
            vue->setPosition(vue->getPosition()+vue->getRotatedVector(MVector3(0,-translation_speed, 0)));
            vue->updateMatrix();

            //window.glbox->redraw();
        }
    }
    else if(!game->isRunning())
    {
        window.inputMethod->callFunction(window.inputMethod->getInputUpdate().c_str());
    }

    if(Maratis::getInstance()->hasTitleChanged())
    {
        main_window->label(Maratis::getInstance()->getWindowTitle());
        Maratis::getInstance()->setTitleChanged(false);
    }

    if(MWindow::getInstance()->getFocus())
    {
        if(game)
        {
            if(!game->isRunning())
                MFilesUpdate::update();
            else
                update_scene_tree();
        }
        else
        {
            MFilesUpdate::update();
        }
    }

    /*if(!game->isRunning())
    {
        Maratis::getInstance()->getPerspectiveVue()->loadSkybox(scene->getCurrentCamera()->getSkybox()->getPath());
    }*/

    window.glbox->redraw();
    Maratis::getInstance()->logicLoop();

    Fl::add_timeout(0.01, update_editor);
}

void GLBox::loadPostEffectsFromGame(MGame* game)
{
    if(game->hasPostEffects())
    {
        m_postProcessor.loadShaderFile(game->getPostProcessor()->getVertexShader(),
                                                            game->getPostProcessor()->getFragmentShader());
        m_postProcessing = true;
        m_postProcessor.eraseTextures();
        m_postProcessor.updateResolution();
    }
    else
    {
        disablePostEffects();
    }
}

void GLBox::draw()
{
    if(!maratis_init)
    {
        Maratis* maratis = Maratis::getInstance();
        maratis->initRenderer();
        MWindow::getInstance()->setViewport(w(), h());

        MRenderingContext * render = MEngine::getInstance()->getRenderingContext();

        if(!current_project.file_path.empty())
        {
            current_project.changed = false;
            current_project.path = current_project.file_path;

    #ifndef WIN32
            current_project.path = current_project.path.erase(current_project.path.find_last_of("/")+1, current_project.path.length());
    #else
            current_project.path = current_project.path.erase(current_project.path.find_last_of("\\")+1, current_project.path.length());
    #endif

            Maratis::getInstance()->loadProject(current_project.file_path.c_str());
            ::window.glbox->loadPostEffectsFromGame(MEngine::getInstance()->getGame());
            current_project.level = maratis->getCurrentLevel();
        }

        update_scene_tree();

        // MLOG_INFO("Render version : " << render->getRendererVersion());

        render->setTextureFilterMode(M_TEX_FILTER_NEAREST, M_TEX_FILTER_NEAREST_MIPMAP_NEAREST);

        // TODO: Don't hardcode this!
        render->setClearColor(MVector4(0.18, 0.32, 0.45, 1));

        m_postProcessor.eraseTextures();
        m_postProcessor.updateResolution();

        maratis_init = true;
        reload_editor = true;
    }

    if(reload_editor)
    {
        reload_editor = false;
    }

    Maratis* maratis = Maratis::getInstance();
    MEngine* engine = MEngine::getInstance();
    MRenderingContext* render = engine->getRenderingContext();

    MWindow::getInstance()->setViewport(w(), h());
    render->setScissor(0, 0, w(), h());

    MGame* game = engine->getGame();
    if(!game->isRunning())
    {
        engine->updateRequests();
        render->setClearColor(MVector4(0.18, 0.32, 0.45, 1));

        render->disableScissorTest();
        if(!m_postProcessing || !m_postProcessor.draw(maratis->getPerspectiveVue()))
            maratis->graphicLoop();

        if(maratis->getSelectedObjectsNumber() > 0)
        {
            MObject3d* camera = maratis->getSelectedObjectByIndex(0);
            if(camera != NULL && camera->getType() == M_OBJECT3D_CAMERA)
            {
                render->setClearColor(static_cast<MOCamera*>(camera)->getClearColor());
                render->setViewport(0,0, w()/3, h()/3);
                render->setScissor(0,0, w()/3, h()/3);
                render->enableScissorTest();

                static_cast<MOCamera*>(camera)->enable();

                render->clear(M_BUFFER_DEPTH | M_BUFFER_COLOR);
                engine->getLevel()->getCurrentScene()->draw(static_cast<MOCamera*>(camera));
                engine->getLevel()->getCurrentScene()->drawObjectsBehaviors();

                render->setScissor(0,0,w(),h());
                render->disableScissorTest();
            }
        }
    }
    else
    {
        maratis->graphicLoop();
    }

    swap_buffers();
}

#define ENDS_WITH(s, e) (s.compare(s.length() - strlen(e), strlen(e), e) == 0)

int GLBox::handle(int event)
{
    //fprintf(stderr, "Handle %d FL_KEYBOARD is %d\n", event, FL_KEYBOARD);
    char key[2] = {0,0};
    MInputContext* input = MEngine::getInstance()->getInputContext();
    switch(event)
    {
    case FL_KEYBOARD:
        {
            if(this != Fl::focus() || Fl::event_ctrl())
                return 0;

            key[0] = toupper(Fl::event_text()[0]);
            MEngine::getInstance()->getInputContext()->downKey(key);

            char keys[27] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};

            for(int i = 0; i < sizeof(keys); i++)
            {
                char c[2] = {0,0};
                c[0] = keys[i];

                if(Fl::get_key(keys[i]))
                {
                    input->downKey(c);
                }
                else
                {
                    input->upKey(c);
                }
            }

            switch(Fl::event_key())
            {
            case FL_Shift_L:
                    input->downKey("LSHIFT");
                break;

            case FL_Shift_R:
                    input->downKey("RSHIFT");
                break;
            }

            Fl_Gl_Window::handle(event);
            return isalpha(Fl::event_text()[0]);
        }
        break;

    case FL_KEYUP:
        {
            if(this != Fl::focus())
                return 0;

            char keys[27] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};

            for(int i = 0; i < sizeof(keys); i++)
            {
                char c[2] = {0,0};
                c[0] = keys[i];

                if(Fl::get_key(keys[i]))
                {
                    input->downKey(c);
                }
                else
                {
                    input->upKey(c);
                }
            }

            switch(Fl::event_key())
            {
            case FL_Shift_L:
                    input->upKey("LSHIFT");
                break;

            case FL_Shift_R:
                    input->upKey("RSHIFT");
                break;
            }

            Fl_Gl_Window::handle(event);
            return 1;
        }
        break;

    case FL_MOVE:
        {
            MMouse* mouse = MMouse::getInstance();
            mouse->setDirection(mouse_x-Fl::event_x(), mouse_y-Fl::event_y());

            mouse_x = Fl::event_x();
            mouse_y = Fl::event_y();

            mouse->setPosition(mouse_x, mouse_y);
        break;
        }
    case FL_MOUSEWHEEL:
        {
            int direction = Fl::event_dy();

            MMouse* mouse = MMouse::getInstance();
            mouse->setWheelDirection(direction);

            MInputContext* input = MEngine::getInstance()->getInputContext();
            input->setAxis("MOUSE_WHEEL", input->getAxis("MOUSE_WHEEL") + direction);

            if(::window.inputMethod == NULL)
            {
                MOCamera * vue = Maratis::getInstance()->getPerspectiveVue();
                vue->setPosition(vue->getPosition() + vue->getRotatedVector(MVector3(0,0,direction*translation_speed)));
                vue->updateMatrix();
            }

            redraw();
            return 1;
        }
        break;

    case FL_FOCUS:
            //Fl::focus(this);
            MWindow::getInstance()->setFocus(true);
            return 1;
        break;

    case FL_UNFOCUS:
            MWindow::getInstance()->setFocus(false);
            return 1;
        break;

    case FL_PUSH:

            Fl::focus(this);

            mouse_x = Fl::event_x();
            mouse_y = Fl::event_y();

            MMouse::getInstance()->setPosition(mouse_x, mouse_y);

            if(Fl::event_button1() && !MEngine::getInstance()->getGame()->isRunning())
            {
                Maratis::getInstance()->selectObjectsInMainView(MEngine::getInstance()->getLevel()->getCurrentScene(), Fl::event_shift() > 0);
                ::window.scene_tree->deselect_all();

                Fl_Tree_Item* item = ::window.scene_tree->root();

                update_name = true;
                while((item = ::window.scene_tree->next(item)) != NULL)
                {
                    if(Maratis::getInstance()->getSelectedObjectsNumber() == 0)
                        break;

                    if(item && !strcmp(item->label(), Maratis::getInstance()->getSelectedObjectByIndex(0)->getName()))
                    {
                        update_name = true;
                        ::window.scene_tree->select(item);
                    }
                }

                redraw();
                ::window.special_tab->redraw();
            }

            {
                    MInputContext* input = MEngine::getInstance()->getInputContext();

                    switch(Fl::event_button())
                    {

                        case FL_MIDDLE_MOUSE:
                        {
                            MMouse::getInstance()->downButton(MMOUSE_BUTTON_MIDDLE);
                            input->onKeyDown("MOUSE_BUTTON_MIDDLE");
                            input->downKey("MOUSE_BUTTON_MIDDLE");
                        }
                        break;

                        case FL_RIGHT_MOUSE:
                        {
                            MMouse::getInstance()->downButton(MMOUSE_BUTTON_RIGHT);
                            input->onKeyDown("MOUSE_BUTTON_RIGHT");
                            input->downKey("MOUSE_BUTTON_RIGHT");
                        }

                        case FL_LEFT_MOUSE:
                        {
                            MMouse::getInstance()->downButton(MMOUSE_BUTTON_LEFT);
                            input->onKeyDown("MOUSE_BUTTON_LEFT");
                            input->downKey("MOUSE_BUTTON_LEFT");
                        }
                    }
            }
            return 1;
        break;

    case FL_RELEASE:
    {
        MInputContext* input = MEngine::getInstance()->getInputContext();

        switch(Fl::event_button())
        {

            case FL_MIDDLE_MOUSE:
            {
                MMouse::getInstance()->upButton(MMOUSE_BUTTON_MIDDLE);
                input->onKeyUp("MOUSE_BUTTON_MIDDLE");
                input->upKey("MOUSE_BUTTON_MIDDLE");
            }
            break;

            case FL_RIGHT_MOUSE:
            {
                MMouse::getInstance()->upButton(MMOUSE_BUTTON_RIGHT);
                input->onKeyUp("MOUSE_BUTTON_RIGHT");
                input->upKey("MOUSE_BUTTON_RIGHT");
            }

            case FL_LEFT_MOUSE:
            {
                MMouse::getInstance()->upButton(MMOUSE_BUTTON_LEFT);
                input->onKeyUp("MOUSE_BUTTON_LEFT");
                input->upKey("MOUSE_BUTTON_LEFT");
            }
        }
        return 1;
    }
        break;

    case FL_DRAG:
        {
            MGame* game = MEngine::getInstance()->getGame();
            if(Fl::event_button3() && ::window.inputMethod == NULL && game->isRunning())
            {
                MOCamera * vue = Maratis::getInstance()->getPerspectiveVue();

                vue->setEulerRotation(vue->getEulerRotation() + MVector3(mouse_y - Fl::event_y(), 0, mouse_x-Fl::event_x())*0.5*rotation_speed);
                vue->updateMatrix();
            }

            MMouse* mouse = MMouse::getInstance();
            mouse->setDirection(mouse_x-Fl::event_x(), mouse_y-Fl::event_y());

            mouse_x = Fl::event_x();
            mouse_y = Fl::event_y();

            mouse->setPosition(mouse_x, mouse_y);
            if(Fl::event_button1() && !game->isRunning())
            {
                Maratis::getInstance()->transformSelectedObjects();
            }

            redraw();
        }
        break;

    case FL_DND_ENTER:
    case FL_DND_DRAG:
    case FL_DND_RELEASE:
        return 1;
        break;

    case FL_PASTE:
            {
                std::string filename = Fl::event_text();

                // Some WMs like to add a '\n' to the path.
                if(ENDS_WITH(filename, "\n"))
                    filename.erase(filename.end()-1);

                std::string suffix = filename;

                // Window does not add 'file://' to the path.
				#ifndef WIN32
                if(filename.find("file://") != 0)
                    return 1;

				filename = filename.substr(7);
				#endif
  
                // Don't try to load anything if path is shorter than the shortest file extension
                if(filename.length() < 4)
                    return 1;

                // Convert path to lowercase
                std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);

                if(ENDS_WITH(suffix, ".mproj") != 0)
                {
                    if(!current_project.path.empty() && !fl_ask("You need to close the current project. Do you want to proceed?"))
                        return 1;

                    current_project.path = filename;
                    current_project.file_path = filename;

                #ifndef WIN32
                    current_project.path = current_project.path.erase(current_project.path.find_last_of("/")+1, current_project.path.length());
                #else
                    current_project.path = current_project.path.erase(current_project.path.find_last_of("\\")+1, current_project.path.length());
                #endif
                    Maratis::getInstance()->loadProject(filename.c_str());
                    loadPostEffectsFromGame(MEngine::getInstance()->getGame());
                    current_project.level = Maratis::getInstance()->getCurrentLevel();
                    reload_editor = true;
                }
                else if(ENDS_WITH(suffix, ".level"))
                {
                    Maratis::getInstance()->loadLevel(filename.c_str());
                    current_project.level = filename;
                }
                else if(ENDS_WITH(suffix, ".mesh"))
                {
                    Maratis::getInstance()->okAddEntity(filename.c_str());
                }
                else if(ENDS_WITH(suffix, ".ttf"))
                {
                    Maratis::getInstance()->okAddFont(filename.c_str());
                }
                else if(ENDS_WITH(suffix, ".wav") || ENDS_WITH(suffix, ".ogg") || ENDS_WITH(suffix, ".aiff"))
                {
                    Maratis::getInstance()->okAddSound(filename.c_str());
                }
                else
                {
                    MLOG_ERROR("Could not load file on drag'n'drop: " << filename);
                    fl_message("Could not load the file you dropped. File type is unknown!");
                }

                update_scene_tree();
                return 1;
            }
        break;
    }

    if(input)
    {
        input->setAxis("MOUSE_X", (static_cast<float>(mouse_x)/w()));
        input->setAxis("MOUSE_Y", (static_cast<float>(mouse_y)/h()));
    }

    Fl_Tree_Item* item = ::window.scene_tree->first_selected_item();

    if(item)
    {
        update_name = false;
        ::window.scene_tree->deselect(item);
        ::window.scene_tree->select(item);
        ::window.special_tab->redraw();
        update_name = true;
    }

    return Fl_Gl_Window::handle(event);
}

void GLBox::resize(int x, int y, int w, int h)
{
    Fl_Gl_Window::resize(x,y,w,h);

    if(m_postProcessing)
    {
        MWindow::getInstance()->setViewport(w, h);
        m_postProcessor.eraseTextures();
        m_postProcessor.updateResolution();
    }
}
