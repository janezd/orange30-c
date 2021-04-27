"""<name> 3D Scatter Plot</name>
"""

from OWWidget import *
from OWGraph3D import *

import OWGUI
import OWColorPalette

import numpy

class OWScatterPlot3D(OWWidget):
    contextHandlers = {"": DomainContextHandler("", ["xAttr", "yAttr", "zAttr"])}
    
    def __init__(self, parent=None, signalManager=None, name="Scatter Plot 3D"):
        OWWidget.__init__(self, parent, signalManager, name)
        
        self.inputs = [("Examples", ExampleTable, self.setData), ("Subset Examples", ExampleTable, self.setSubsetData)]
        self.outputs = []
        
        self.xAttr = 0
        self.yAttr = 0
        self.zAttr = 0
        
        self.colorAttr = None
        self.sizeAttr = None
        self.labelAttr = None
        
        self.pointSize = 5
        self.alphaValue = 255
        
        # ###
        # GUI
        # ###
        
#        box = OWGUI.widgetBox(self.controlArea, "Axes", addSpace=True)
        self.xAttrCB = OWGUI.comboBox(self.controlArea, self, "xAttr", box="X-axis Attribute",
                                      tooltip="Attribute to plot on X axis.",
                                      callback=self.onAxisChange
                                      )
        
        self.yAttrCB = OWGUI.comboBox(self.controlArea, self, "yAttr", box="Y-axis Attribute",
                                      tooltip="Attribute to plot on Y axis.",
                                      callback=self.onAxisChange
                                      )
        
        self.zAttrCB = OWGUI.comboBox(self.controlArea, self, "zAttr", box="Z-axis Attribute",
                                      tooltip="Attribute to plot on Z axis.",
                                      callback=self.onAxisChange
                                      )
        
        self.colorAttrCB = OWGUI.comboBox(self.controlArea, self, "colorAttr", box="Point color",
                                          tooltip="Attribute to use for point color",
                                          callback=self.onAxisChange)
        
        self.sizeAttrCB = OWGUI.comboBox(self.controlArea, self, "sizeAttr", box="Point Size",
                                         tooltip="Attribute to use for pointSize",
                                         callback=self.onAxisChange,
                                         )
        OWGUI.hSlider(self.controlArea, self, "pointSize", box="Max. point size",
                      minValue=1, maxValue=10,
                      tooltip="Maximum point size",
                      callback=self.onAxisChange
                      )
        
#        self.alphaSlizer = OWGUI.hSlider(self.controlArea, self, "alphaValue", box="Transparency",
#                                         minValue=10, maxValue=255,
#                                         tooltip="Point transparency value",
#                                         callback=self.onAxisChange)
        
        #TODO: jittering options
        
        OWGUI.rubber(self.controlArea)
        
        self.graph = OWGraph3D(self)
        self.mainArea.layout().addWidget(self.graph)
        
        self.data = None
        self.subsetData = None
        
        self.resize(800, 600)
    
    
    def setData(self, data=None):
        self.closeContext("")
        self.data = data
        self.xAttrCB.clear()
        self.yAttrCB.clear()
        self.zAttrCB.clear()
        self.colorAttrCB.clear()
        self.sizeAttrCB.clear()
        if self.data is not None:
            self.allAttrs = data.domain.variables + list(data.domain.getmetas().values())
            self.axisCandidateAttrs = [attr for attr in self.allAttrs if attr.varType in [orange.VarTypes.Continuous, orange.VarTypes.Discrete]]
            
            self.colorAttrCB.addItem("<None>")
            self.sizeAttrCB.addItem("<None>")
            icons = OWGUI.getAttributeIcons() 
            for attr in self.axisCandidateAttrs:
                self.xAttrCB.addItem(icons[attr.varType], attr.name)
                self.yAttrCB.addItem(icons[attr.varType], attr.name)
                self.zAttrCB.addItem(icons[attr.varType], attr.name)
                self.colorAttrCB.addItem(icons[attr.varType], attr.name)
                self.sizeAttrCB.addItem(icons[attr.varType], attr.name)
            
            array, c, w = self.data.toNumpyMA()
            if len(c):
                array = numpy.hstack((array, c.reshape(-1,1)))
            self.dataArray = array
            
            self.xAttr, self.yAttr, self.zAttr = numpy.min([[0, 1, 2], [len(self.axisCandidateAttrs) - 1]*3], axis=0)
            self.colorAttr = max(len(self.axisCandidateAttrs) - 1, 0)
             
            self.openContext("", data)
            
        
    def setSubsetData(self, data=None):
        self.subsetData = data
        
        
    def handleNewSignals(self):
        self.updateGraph()
        
        
    def onAxisChange(self):
        if self.data is not None:
            self.updateGraph()
            
    def updateGraph(self):
        if self.data is None:
            return
        
        xInd, yInd, zInd = self.getAxesIndices()
        X, Y, Z, mask = self.getAxisData(xInd, yInd, zInd)
        
        if self.colorAttr > 0:
            colorAttr = self.axisCandidateAttrs[self.colorAttr -1]
            C = self.dataArray[:, self.colorAttr - 1]
        
            if colorAttr.varType == orange.VarTypes.Discrete:
                palette = OWColorPalette.ColorPaletteHSV(len(colorAttr.values))
                colors = [palette[int(value)] for value in C.ravel()]
                colors = [[c.red()/255., c.green()/255., c.blue()/255.] for c in colors]
            else:
                palette = OWColorPalette.ColorPaletteBW()
                maxC, minC = numpy.max(C), numpy.min(C)
                C = (C - minC) / (maxC - minC)
                colors = [palette[value] for value in C.ravel()]
                colors = [[c.red()/255., c.green()/255., c.blue()/255.] for c in colors]
        else:
            colors = "b"
            
        if self.sizeAttr > 0:
            sizeAttr = self.axisCandidateAttrs[self.sizeAttr - 1]
            S = self.dataArray[:, self.sizeAttr - 1]
            if sizeAttr.varType == orange.VarTypes.Discrete:
                sizes = [(v + 1) * len(sizeAttr.values) / (11 - self.pointSize) for v in S]
            else:
                min, max = numpy.min(S), numpy.max(S)
                sizes = [(v - min) * self.pointSize / (max-min) for v in S]
        else:
            sizes = 1
        
        self.graph.clear()
        self.graph.scatter(X, Y, Z, colors, sizes)
        
        
    def getAxisData(self, xInd, yInd, zInd):
        array = self.dataArray
        X, Y, Z =  array[:, xInd], array[:, yInd], array[:, zInd]
        return X, Y, Z, None
    
    def getAxesIndices(self):
        return self.xAttr, self.yAttr, self.zAttr
        
        
if __name__ == "__main__":
    app = QApplication(sys.argv)
    w = OWScatterPlot3D()
    data = orange.ExampleTable("../../doc/datasets/iris")
    w.setData(data)
    w.handleNewSignals()
    w.show()
    app.exec_()
        
        
        
            
    
        
        
        