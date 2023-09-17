/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once


class GameState
{
    public:
        GameState();

        typedef enum 
        {
            STATE_LOADING,
            STATE_SIMULATION,
            STATE_GUI
        } SimulationState;


        void pause(bool state);
        bool paused() const;

    private:
        SimulationState state_;
        bool coop_;
        bool paused_;
};
