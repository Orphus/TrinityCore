/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Creature.h"
#include "MapManager.h"
#include "ConfusedMovementGenerator.h"
#include "PathFinderMovementGenerator.h"
#include "VMapFactory.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"

#ifdef MAP_BASED_RAND_GEN
#define rand_norm() unit.rand_norm()
#define urand(a, b) unit.urand(a, b)
#endif

template<class T>
void ConfusedMovementGenerator<T>::Initialize(T &unit)
{
    unit.GetPosition(_x, _y, _z);
    unit.StopMoving();
    unit.SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    unit.AddUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
}

template<class T>
void ConfusedMovementGenerator<T>::Reset(T &unit)
{
    _nextMoveTime.Reset(0);
    unit.AddUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
    unit.StopMoving();
}

template<class T>
bool ConfusedMovementGenerator<T>::Update(T &unit, const uint32 &diff)
{
    if (unit.HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DISTRACTED))
        return true;

    if (_nextMoveTime.Passed())
    {
        // currently moving, update location
        unit.AddUnitState(UNIT_STATE_CONFUSED_MOVE);

        if (unit.movespline->Finalized())
            _nextMoveTime.Reset(urand(800, 1500));
    }
    else
    {
        // waiting for next move
        _nextMoveTime.Update(diff);
        if (_nextMoveTime.Passed())
        {
            // start moving
            unit.AddUnitState(UNIT_STATE_CONFUSED_MOVE);

            float x = _x + 10.0f*((float)rand_norm() - 0.5f);
            float y = _y + 10.0f*((float)rand_norm() - 0.5f);
            float z = _z + 0.5f;

            Trinity::NormalizeMapCoord(x);
            Trinity::NormalizeMapCoord(y);

            unit.UpdateAllowedPositionZ(x, y, z);

            if (z <= INVALID_HEIGHT)
                _z = unit.GetBaseMap()->GetHeight(unit.GetPhaseMask(), x, y, MAX_HEIGHT) + 2.0f;

            PathFinderMovementGenerator path(&unit);
            path.SetPathLengthLimit(30.0f);
            path.SetUseStrightPath(false);

            if (!unit.IsWithinLOS(x, y, z) || !path.Calculate(x, y, z) || path.GetPathType() & PATHFIND_NOPATH)
            {
                _nextMoveTime.Reset(urand(200, 500));
                return true;
            }

            Movement::MoveSplineInit init(unit);
            init.MovebyPath(path.GetPath());
            init.SetWalk(true);
            init.Launch();
        }
    }

    return true;
}

template<>
void ConfusedMovementGenerator<Player>::Finalize(Player &unit)
{
    unit.RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    unit.ClearUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
    unit.StopMoving();
}

template<>
void ConfusedMovementGenerator<Creature>::Finalize(Creature &unit)
{
    unit.RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    unit.ClearUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_CONFUSED_MOVE);
    if (unit.getVictim())
        unit.SetTarget(unit.getVictim()->GetGUID());
}

template void ConfusedMovementGenerator<Player>::Initialize(Player &player);
template void ConfusedMovementGenerator<Creature>::Initialize(Creature &creature);
template void ConfusedMovementGenerator<Player>::Reset(Player &player);
template void ConfusedMovementGenerator<Creature>::Reset(Creature &creature);
template bool ConfusedMovementGenerator<Player>::Update(Player &player, const uint32 &diff);
template bool ConfusedMovementGenerator<Creature>::Update(Creature &creature, const uint32 &diff);

