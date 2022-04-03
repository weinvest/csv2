#!/usr/bin/env python3
import os
import sys
import libpycsv2 as csv
import npyscreen as nps
import logging

from wggridmultitles import GridMulTitles

class CSViewer(nps.Form):
    
    def collect_titles(self):
        self.titles = []
        for h in self.csv_file.header():
            this_title = []
            for c in h:
                this_title.append(str(c))
            self.titles.append(this_title)
        logging.info(f'headers:{self.titles}')

    def cal_col_width(self, max_rows):
        self.column_widths = [0]*len(self.titles[0])
        for h in self.titles:
            for i,c in enumerate(h):
                self.column_widths[i] = max(self.column_widths[i], len(c))
        
        considered = set()
        for ir,r in enumerate(self.csv_file):
            if ir >= max_rows or len(considered) == len(self.titles):
                break
            
            for ic, c in enumerate(r):
                if 0 == ic:
                    considered.add(c.get_prefix(':'))
                self.column_widths[i] = max(self.column_widths[ic], len(str(c)))
        return self.column_widths

    def create(self):
        self.workflow = 1

        self.csv_file = csv.CommaHeaderCSV()
        self.csv_file.mmap(self.parentApp.get_csv_path())
        self.collect_titles()

        self.screen_height, self.screen_width = self._max_physical()    
        border_width=1
        border_height=1
        width = self.screen_width-4*border_width
        height = self.screen_height-2*border_height
        column_widths=self.cal_col_width(50)
        column_width=0
        for col_width in column_widths:
            column_width = max(col_width, column_width)
        
        column_width += 2
        logging.info(f'width:{width}, height:{height}, col_width:{column_width}')
        self.csv_wg = self.add(GridMulTitles
                , name=self.parentApp.get_app_name()
                , columns=self.csv_file.cols()
                , column_width = column_width
                , max_height=height
                , max_width=width
                , height=height
                , width=width
                , col_titles=self.titles
                , rely = border_height
                , relx = border_width
                # , select_whole_line=True
                , values=self.csv_file
                )

    def resize(self):
        super(CSViewer, self).resize()
        self.screen_height, self.screen_width = self._max_physical()    
        border_width=1
        border_height=1
        width = self.screen_width-4*border_width
        height = self.screen_height-2*border_height
        self.csv_wg.relx = border_width
        self.csv_wg.rely = border_height
        self.csv_wg.max_width = width
        self.csv_wg.max_height = height

    def while_waiting(self):
        self.set_value(self.workflow)
        self.display()

class CSViewerApp(nps.NPSAppManaged):
    def __init__(self, app_name, csv_path):
        self.app_name = app_name
        self.csv_path = csv_path
        super(CSViewerApp, self).__init__()

    def onStart(self):
        self.keypress_timeout_default = 5
        self.addForm('MAIN', CSViewer)

    def get_app_name(self):
        return self.app_name

    def get_csv_path(self):
        return self.csv_path

    def while_waiting(self):
        pass

if __name__ == '__main__':
    app_name = 'CSViewer'
    
    import getpass
    user_name = getpass.getuser()
    logging.basicConfig(filename=f'/tmp/{user_name}.csv_viewer.log', level=logging.INFO)

    myApp = CSViewerApp(app_name, sys.argv[1])
    myApp.run()
