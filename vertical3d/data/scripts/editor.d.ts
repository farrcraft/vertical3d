// The commands a script can run in the editor. The editor writes this file from its own
// commands, and a test checks the two agree.

declare namespace console {
    function log(...values: unknown[]): void;
}

declare namespace create {
    function group(args?: { items?: string[] }): void;
}

declare namespace create.curve {
    function bezier(): void;
    function catmull_rom(): void;
    function nurbs(): void;
    function poly(): void;
}

declare namespace create.light {
    function distant(): void;
    function point(): void;
    function spot(): void;
}

declare namespace create.ortho {
    function camera(): void;
}

declare namespace create.perspective {
    function camera(): void;
}

declare namespace create.poly {
    function cone(): void;
    function cube(): void;
    function cylinder(): void;
    function plane(): void;
}

declare namespace edit {
    function redo(): void;
    function undo(): void;
}

declare namespace project {
    function save(): void;
}

declare namespace render {
    function batch(): void;
    function settings(): void;
    function stop(): void;
    function view(): void;
}

declare namespace select.mask {
    function camera(): void;
    function curve(): void;
    function edge(): void;
    function face(): void;
    function handle(): void;
    function light(): void;
    function mesh(): void;
    function object(): void;
    function vertex(): void;
}

declare namespace timeline {
    function auto_key(args?: { on?: boolean }): void;
    function delete_keys(): void;
    function set(args?: { items?: string[]; frame?: number }): void;
}

declare namespace timeline.playback {
    function first(): void;
    function first_key(): void;
    function last(): void;
    function last_key(): void;
    function next(): void;
    function next_key(): void;
    function play(): void;
    function previous(): void;
    function previous_key(): void;
    function stop(): void;
}

declare namespace transform {
    function rotate(): void;
    function scale(): void;
    function select(): void;
    function translate(): void;
}

declare namespace transform.poly {
    function extrude_face(args?: { parts?: string[] }): void;
    function inset_face(args?: { parts?: string[] }): void;
    function merge_vertices(args?: { parts?: string[] }): void;
    function split_edge(args?: { parts?: string[] }): void;
    function split_face(args?: { parts?: string[] }): void;
}
