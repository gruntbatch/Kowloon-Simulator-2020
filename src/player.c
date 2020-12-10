#include "player.h"


#include "events.h"


#define EYE_HEIGHT 1.70


/* TODO Mouse sensitivity should be set via user preferences */
#define MOUSE_SPEED_X 5.0f
#define MOUSE_SPEED_Y 5.0f


#define MOVEMENT_SPEED 5.0f


Agent player;
static float pitch = -90.0f;
static float yaw = 0.0f;    


void SpawnPlayer(Area area) {
    player = SpawnAgent(area);
}


void PlayerWalkabout(float delta_time) {
    union Vector2 look = GetLook();
    pitch += look.y * MOUSE_SPEED_Y * delta_time;
    pitch = clampf(-160.0f, pitch, -20.f);

    yaw += look.x * MOUSE_SPEED_X * delta_time;

    union Matrix4 yaw_matrix = MulM4(GetAgentRotation(player),
				     Rotation(AxisAngle(Vector3(0, 0, 1), to_radians(yaw))));

    union Vector2 move = GetMove();
    union Vector2 goal = Transform4(InvertM4(yaw_matrix),
				    Vector4(move.x, move.y, 0, 1)).xy;

    MoveAgent(player, goal, delta_time);
}


union Matrix4 GetPlayerView(void) {
    return MulM4(MulM4(Rotation(AxisAngle(Vector3(1, 0, 0), to_radians(pitch))),
		       MulM4(GetAgentRotation(player),
			     Rotation(AxisAngle(Vector3(0, 0, 1), to_radians(yaw))))),
		 InvertM4(Translation(Add3(GetAgentPosition(player),
					   Vector3(0, 0, EYE_HEIGHT)))));

}


Area GetPlayerArea(void) {
    return GetAgentArea(player);
}


void DrawPlayer(float radius) {
    DrawAgent(player, radius);
}
