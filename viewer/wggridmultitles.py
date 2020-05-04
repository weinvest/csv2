#!/usr/bin/env python3
# encoding: utf-8
import curses
import npyscreen as nps
import logging

class GridMulTitles(nps.SimpleGrid):
    _col_widgets = nps.Textfield
    def __init__(self, screen, col_titles, *args, **keywords):
        if col_titles:
            self.col_titles = col_titles
        else:
            self.col_titles = []

        self.need_line_no = True
        GridMulTitles.additional_y_offset = len(self.col_titles) + 1
        super(GridMulTitles, self).__init__(screen, *args, **keywords)
    
    def make_contained_widgets(self):
        super(GridMulTitles, self).make_contained_widgets()
        self._my_col_titles = []

        for y_offset in range(len(self.col_titles)):
            self._my_col_titles.append([])
            for title_cell in range(self.columns):
                x_offset = title_cell * (self._column_width+self.col_margin)
                self._my_col_titles[-1].append(self._col_widgets(self.parent,
                    rely=self.rely + y_offset, relx = self.relx + x_offset,
                    width=self._column_width, height=1))

    def set_up_handlers(self):
        super(GridMulTitles, self).set_up_handlers()
        self.handlers.update({
            "0" : self.h_move_cell_beg,
            "$" : self.h_move_cell_end,
            "H" : self.h_move_cell_beg,
            "L" : self.h_move_cell_end,
            "^F": self.h_move_page_down,
            "^B": self.h_move_page_up,
            "n": self.h_show_line_no,          
        })

    def custom_print_cell(self, cell, value):
        if -1 == cell.grid_current_value_index:
            return

        row, col = cell.grid_current_value_index
        if 0 == col and self.need_line_no:
            cell.value = f'{row}: {value}'

    def update(self, clear=True):
        super(GridMulTitles, self).update(clear = True)
        
        for r in range(len(self.col_titles)):
            _title_counter = 0
            for title_cell in self._my_col_titles[r]:
                try:
                    title_text = self.col_titles[r][self.begin_col_display_at+_title_counter]
                    #logging.info(f'{r},{self.begin_col_display_at},{_title_counter}:{title_text}')
                except IndexError:
                    title_text = None
                self.update_title_cell(title_cell, title_text)
                _title_counter += 1
            
        self.parent.curses_pad.hline(self.rely+len(self.col_titles), self.relx, curses.ACS_HLINE, self.max_width+3*self.col_margin)
    
    def update_title_cell(self, cell, cell_title):
        cell.value = cell_title
        cell.update()

    def h_move_cell_beg(self, inpt):
        self.edit_cell[1] = 0
        self.begin_col_display_at = 0
        
        self.on_select(inpt)
    
    def h_move_cell_end(self, inpt):
        self.edit_cell[1] = self.columns - 1
        self.begin_col_display_at = self.columns-1
        self.on_select(inpt)
    
    def h_show_line_no(self, inpt):
        self.need_line_no = False if self.need_line_no else True
        
