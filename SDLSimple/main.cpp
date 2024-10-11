/**
* Author: [Amy Li]
* Assignment: Pong Clone
* Date due: 2024-10-12, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 2.0f;

constexpr int WINDOW_WIDTH  = 640 * WINDOW_SIZE_MULT,
              WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

//constexpr float BG_RED     = 0.9765625f,
//                BG_GREEN   = 0.97265625f,
//                BG_BLUE    = 0.9609375f,
//                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
              VIEWPORT_Y = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL    = 0;
constexpr GLint TEXTURE_BORDER     = 0;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char PLAYER_ONE_FILEPATH[] = "player1.png",
               PLAYER_TWO_FILEPATH[]  = "player2.png",
               BALL_FILEPATH[]  = "ball.png";

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3 INIT_SCALE_BALL      = glm::vec3(0.3f, 0.3f, 0.0f),
                    INIT_POS_BALL        = glm::vec3(0.0f, 0.0f, 0.0f),
                    INIT_SCALE_PLAYER    = glm::vec3(2.02f, 1.35f, 0.0f),
                    INIT_POS_PLAYER_ONE  = glm::vec3(-4.0f, 0.0f, 0.0f),
                    INIT_POS_PLAYER_TWO  = glm::vec3(4.0f, 0.0f, 0.0f);

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix,
          g_player_one_matrix,
          g_player_two_matrix,
          g_projection_matrix,
          g_ball_matrix;

float g_previous_ticks = 0.0f;

float g_rot_angle = 0.0f;

float screen_height_boundary = 3.75f;
float paddle_height = INIT_SCALE_PLAYER.y;
float paddle_width = INIT_SCALE_PLAYER.x;
float ball_radius = INIT_SCALE_BALL.x * 0.5f;

bool twoplayers = true;
bool flip_x_mov = false;
bool flip_y_mov = false;

constexpr float BALL_SPEED = 1.3f,
                PLAYER_SPEED = 3.0f,
                ROT_SPEED = 100.0f;

int player_one_score = 0,
    player_two_score = 0;

GLuint g_player_one_texture_id;
GLuint g_player_two_texture_id;
GLuint g_ball_texture_id;

glm::vec3 g_player_one_position  = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player_one_movement  = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_player_two_position  = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player_two_movement  = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);


void initialise();
void process_input();
void paddle_wall_collision();
void paddle_ball_collision();
void ball_wall_collision();
void update();
void render();
void shutdown();


GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pong",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr) shutdown();

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_player_one_matrix = glm::mat4(1.0f);
    g_player_two_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
        
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(0.8f, 1.0f, 0.8f, 1.0f);
    
    g_player_one_texture_id = load_texture(PLAYER_ONE_FILEPATH);
    g_player_two_texture_id = load_texture(PLAYER_TWO_FILEPATH);
    g_ball_texture_id = load_texture(BALL_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_ball_movement = glm::vec3(0.0f);
    g_player_one_movement = glm::vec3(0.0f);
    g_player_two_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;
                    case SDLK_w:
                        g_player_one_movement.y = 1.0f;
                        break;
                    case SDLK_s:
                        g_player_one_movement.y = -1.0f;
                        break;
                    case SDLK_UP:
                        g_player_two_movement.y = 1.0f;
                        break;
                    case SDLK_DOWN:
                        g_player_two_movement.y = -1.0f;
                        break;
                    case SDLK_t:
                        twoplayers = false;
                        break;
                }
            case SDL_KEYUP:
                switch (event.key.keysym.sym)
                {
                    case SDLK_w:
                    case SDLK_s:
                        g_player_one_movement.y = 0.0f;
                        break;
                    case SDLK_UP:
                    case SDLK_DOWN:
                        g_player_two_movement.y = 0.0f;
                        break;
                }
                break;
            default:
                break;
        }
    }
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    if (glm::length(g_ball_movement) > 1.0f)
    {
        g_ball_movement = glm::normalize(g_ball_movement);
    }
    // player 1 keys
    if (key_state[SDL_SCANCODE_W])
    {
        g_player_one_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        g_player_one_movement.y = -1.0f;
    }
    //player 2 keys
    if (!twoplayers)
    {
        if (g_ball_position.y > g_player_two_position.y + INIT_SCALE_PLAYER.y * 0.5f)
        {
            g_player_two_movement.y = 1.0f;
        }
        else if (g_ball_position.y < g_player_two_position.y - INIT_SCALE_PLAYER.y * 0.5f)
        {
            g_player_two_movement.y = -1.0f;
        }
        else
        {
            g_player_two_movement.y = 0.0f;
        }
    }
    else {
        if (key_state[SDL_SCANCODE_UP])
        {
            g_player_two_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN])
        {
            g_player_two_movement.y = -1.0f;
        }
    }
    paddle_wall_collision();
    paddle_ball_collision();
    ball_wall_collision();
}

void paddle_wall_collision() {
    if (g_player_one_position.y > (screen_height_boundary) - (INIT_SCALE_PLAYER.y * 0.5f))
    {
        g_player_one_position.y = (screen_height_boundary) - (INIT_SCALE_PLAYER.y * 0.5f);
        g_player_one_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    if (g_player_one_position.y < (-screen_height_boundary) + (INIT_SCALE_PLAYER.y * 0.5f))
    {
        g_player_one_position.y = (-screen_height_boundary) + (INIT_SCALE_PLAYER.y * 0.5f);
        g_player_one_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    if (g_player_two_position.y > (screen_height_boundary) - (INIT_SCALE_PLAYER.y * 0.5f))
    {
        g_player_two_position.y = (screen_height_boundary) - (INIT_SCALE_PLAYER.y * 0.5f);
        g_player_two_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    if (g_player_two_position.y < (-screen_height_boundary) + (INIT_SCALE_PLAYER.y * 0.5f))
    {
        g_player_two_position.y = (-screen_height_boundary) + (INIT_SCALE_PLAYER.y * 0.5f);
        g_player_two_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

void paddle_ball_collision(){
    
    float x_one_distance = fabs((g_player_one_position.x + INIT_POS_PLAYER_ONE.x) - (g_ball_position.x + INIT_POS_BALL.x)) -
    ((INIT_SCALE_BALL.x + INIT_SCALE_PLAYER.x) / 2.0f);
    float y_one_distance = fabs((g_player_one_position.y + INIT_POS_PLAYER_ONE.y) - (g_ball_position.y + INIT_POS_BALL.y)) -
    ((INIT_SCALE_BALL.y + INIT_SCALE_PLAYER.y) / 2.0f);
        
    if (x_one_distance < 0 && y_one_distance <= 0){
        flip_x_mov = !flip_x_mov;
        flip_y_mov = !flip_y_mov;
        g_ball_movement.x *= -1;;
    }
    
    float x_two_distance = fabs((g_player_two_position.x + INIT_POS_PLAYER_TWO.x) - (g_ball_position.x + INIT_POS_BALL.x)) -
    ((INIT_SCALE_BALL.x + INIT_SCALE_PLAYER.x) / 2.0f);
    float y_two_distance = fabs((g_player_two_position.y + INIT_POS_PLAYER_TWO.y) - (g_ball_position.y + INIT_POS_BALL.y)) -
    ((INIT_SCALE_BALL.y + INIT_SCALE_PLAYER.y) / 2.0f);

    if (x_two_distance < 0 && y_two_distance <= 0){
        flip_x_mov = !flip_x_mov;
        flip_y_mov = !flip_y_mov;
        g_ball_movement.x *= -1;;
    }

}

void ball_wall_collision() {
    if (g_ball_position.x - ball_radius <= -5.0f)
    {
        player_two_score++;
        std::cout << "player 2 wins\n";
        g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
        g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    else if (g_ball_position.x + ball_radius >= 5.0f)
    {
        player_one_score++;
        std::cout << "player 1 wins\n";
        g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
        g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    if (g_ball_position.y + ball_radius >= screen_height_boundary)
    {
        flip_y_mov = true;
    }
    else if (g_ball_position.y - ball_radius <= -screen_height_boundary)
    {
        flip_y_mov= false;
    }
    
    if (flip_x_mov && flip_y_mov)
    {
        g_ball_movement = glm::vec3(-BALL_SPEED, -BALL_SPEED, 0.0f);
    }
    else if (flip_x_mov)
    {
        g_ball_movement = glm::vec3(-BALL_SPEED, BALL_SPEED, 0.0f);
    }
    else if (flip_y_mov)
    {
        g_ball_movement = glm::vec3(BALL_SPEED, -BALL_SPEED, 0.0f);
    }
    else
    {
        g_ball_movement = glm::vec3(BALL_SPEED, BALL_SPEED, 0.0f);
    }
}

void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    g_player_one_matrix = glm::mat4(1.0f);
    g_player_one_matrix = glm::translate(g_player_one_matrix, INIT_POS_PLAYER_ONE);
    g_player_one_matrix = glm::translate(g_player_one_matrix, g_player_one_position);
    g_player_one_position += g_player_one_movement * PLAYER_SPEED * delta_time;

    g_player_two_matrix = glm::mat4(1.0f);
    g_player_two_matrix = glm::translate(g_player_two_matrix, INIT_POS_PLAYER_TWO);
    g_player_two_matrix = glm::translate(g_player_two_matrix, g_player_two_position);
    g_player_two_position += g_player_two_movement * PLAYER_SPEED * delta_time;

    g_ball_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::translate(g_ball_matrix, INIT_POS_BALL);

    ball_wall_collision();
    paddle_ball_collision();
    
    // win & end game
    if (player_one_score == 5 || player_two_score == 5) {
        g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    
    g_ball_position += g_ball_movement * BALL_SPEED * delta_time;
    g_ball_matrix = glm::translate(g_ball_matrix, g_ball_position);
    
    g_rot_angle += ROT_SPEED * delta_time;
    g_ball_matrix = glm::rotate(g_ball_matrix, glm::radians(g_rot_angle), glm::vec3(0.0f, 0.0f, 1.0f));
    
    g_player_one_matrix = glm::scale(g_player_one_matrix, INIT_SCALE_PLAYER);
    g_player_two_matrix = glm::scale(g_player_two_matrix, INIT_SCALE_PLAYER);
    g_ball_matrix  = glm::scale(g_ball_matrix, INIT_SCALE_BALL);
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_ball_matrix, g_ball_texture_id);
    draw_object(g_player_one_matrix, g_player_one_texture_id);
    draw_object(g_player_two_matrix, g_player_two_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
