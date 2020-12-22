import itertools
import operator


def get_x_ticks_positions(positions: dict, bar_width: float, inner_spacer: float, spacer: float):
    group_widths = []
    x_pos_start = []
    x_pos_end = []
    for key, group in itertools.groupby(positions.values(), operator.itemgetter(0)):
        group_widths.append(len(list(group)) * (bar_width + inner_spacer) - inner_spacer)
    for w in group_widths:
        if not x_pos_start:
            x_pos_start.append(0 + spacer)
        else:
            x_pos_start.append(x_pos_end[-1] + spacer)
        x_pos_end.append(x_pos_start[-1] + w)
    result = list(map(operator.add, x_pos_start, [w / 2 for w in group_widths]))
    return result, x_pos_start


def get_x_position(group_pos: tuple, x_start: list, bar_width: float, inner_spacer: float) -> int:
    return x_start[group_pos[0]] + (group_pos[1] * (bar_width + inner_spacer)) + (bar_width / 2)
