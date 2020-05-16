/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */
#define _USE_MATH_DEFINES
#include <cmath>

#include "sprite_picture.h"
#include "main_data.h"
#include "game_pictures.h"
#include "game_battle.h"
#include "game_screen.h"
#include "player.h"
#include "bitmap.h"


// Applied to ensure that all pictures are above "normal" objects on this layer
constexpr int z_mask = (1 << 16);

Sprite_Picture::Sprite_Picture(int pic_id, Drawable::Flags flags)
	: Sprite(flags), pic_id(pic_id)
{
}

void Sprite_Picture::Draw(Bitmap& dst) {
	const auto& pic = Main_Data::game_pictures->GetPicture(pic_id);
	const auto& data = pic.data;

	auto& bitmap = GetBitmap();

	if (!bitmap || data.name.empty()) {
		return;
	}

	const bool is_battle = Game_Battle::IsBattleRunning();

	if (is_battle ? !pic.IsOnBattle() : !pic.IsOnMap()) {
		return;
	}

	// RPG Maker 2k3 1.12: Spritesheets
	if (Player::IsRPG2k3E()
			&& pic.NumSpriteSheetFrames() > 1
			&& last_spritesheet_frame != data.spritesheet_frame)
	{
		last_spritesheet_frame = data.spritesheet_frame;

		const int sw = bitmap->GetWidth() / data.spritesheet_cols;
		const int sh = bitmap->GetHeight() / data.spritesheet_rows;
		const int sx = sw * ((data.spritesheet_frame) % data.spritesheet_cols);
		const int sy = sh * ((data.spritesheet_frame) / data.spritesheet_cols % data.spritesheet_rows);

		SetSrcRect(Rect{ sx, sy, sw, sh });
	}

	int x = data.current_x;
	int y = data.current_y;
	if (data.flags.affected_by_shake) {
		x -= Main_Data::game_screen->GetShakeOffsetX();
		y -= Main_Data::game_screen->GetShakeOffsetY();
	}

	SetX(x);
	SetY(y);
	if (Player::IsMajorUpdatedVersion()) {
		// Battle Animations are above pictures
		int priority = 0;
		if (is_battle) {
			priority = Drawable::GetPriorityForBattleLayer(data.battle_layer);
		} else {
			priority = Drawable::GetPriorityForMapLayer(data.map_layer);
		}
		if (priority > 0) {
			SetZ(priority + z_mask + data.ID);
		}
	} else {
		// Battle Animations are below pictures
		SetZ(Priority_PictureOld + data.ID);
	}
	SetZoomX(data.current_magnify / 100.0);
	SetZoomY(data.current_magnify / 100.0);

	auto sr = GetSrcRect();
	SetOx(sr.width / 2);
	SetOy(sr.height / 2);

	SetAngle(data.effect_mode != lcf::rpg::SavePicture::Effect_wave ? data.current_rotation * (2 * M_PI) / 256 : 0.0);
	SetWaverPhase(data.effect_mode == lcf::rpg::SavePicture::Effect_wave ? data.current_waver * (2 * M_PI) / 256 : 0.0);
	SetWaverDepth(data.effect_mode == lcf::rpg::SavePicture::Effect_wave ? data.current_effect_power * 2 : 0);

	// Only older versions of RPG_RT apply the effects of current_bot_trans chunk.
	const bool use_bottom_trans = (Player::IsRPG2k3() && !Player::IsRPG2k3E());
	const auto top_trans = data.current_top_trans;
	const auto bottom_trans = use_bottom_trans ? data.current_bot_trans : top_trans;

	SetOpacity(
		(int)(255 * (100 - top_trans) / 100),
		(int)(255 * (100 - bottom_trans) / 100));

	if (bottom_trans != top_trans) {
		SetBushDepth(GetHeight() / 2);
	} else {
		SetBushDepth(0);
	}

	auto tone = Tone((int) (data.current_red * 128 / 100),
			(int) (data.current_green * 128 / 100),
			(int) (data.current_blue * 128 / 100),
			(int) (data.current_sat * 128 / 100));
	if (data.flags.affected_by_tint) {
		auto screen_tone = Main_Data::game_screen->GetTone();
		tone = Blend(tone, screen_tone);
	}
	SetTone(tone);

	if (data.flags.affected_by_flash) {
		SetFlashEffect(Main_Data::game_screen->GetFlashColor());
	}

	Sprite::Draw(dst);
}


