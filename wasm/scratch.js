'use strict';

window.setTimeout(() => {
//


  function assert(condition, message) {
    if (!condition) {
      throw new Error(message || "Assertion failed.");
    }
  }

  class Terminal {
    constructor(prompt = '', properties = {canvasId: 'canvas'}) {
      assert(properties.canvasId != undefined);
      
      this.canvas = document.getElementById(properties.canvasId);
      assert(this.canvas);
      
      this.prompt = prompt;
      
      this.canvas.width = this.canvas.getBoundingClientRect().width;
      this.canvas.height = this.canvas.getBoundingClientRect().height;
      
      this.ctx = this.canvas.getContext('2d');
      this.ctx.font = properties.font || '14px monospace';
      
      // metrics
      
      const text = 'g{^Mwy';
      const metrics = this.ctx.measureText(text);
      
      this.textMetrics = {
        charWidth: metrics.width / text.length,
        charHeight: metrics.actualBoundingBoxAscent,
        lineSpacing: metrics.actualBoundingBoxDescent + 1
      };
      
      this.position = {
        col: this.prompt.length,
        y: this.textMetrics.charHeight
      };
      
      this.cursor = {
        width: this.textMetrics.charWidth,
        negativeHeight: -this.textMetrics.charHeight,
        positiveHeight: this.textMetrics.lineSpacing,
        height: this.textMetrics.charHeight + this.textMetrics.lineSpacing
      };
      
      // buffer to act as text storage
      this.buffer = prompt;
      this.ctx.fillText(this.buffer, 0, this.position.y);
      this.fillCursor();
    }
    
    setBlack() {
      this.ctx.fillStyle = 'black';
    }
    
    setWhite() {
      this.ctx.fillStyle = 'white';
    }
    
    fillCursor() {
      this.setBlack();
      this.ctx.fillRect(
        this.position.col * this.textMetrics.charWidth, this.position.y + this.cursor.negativeHeight, 
        this.cursor.width, this.cursor.height
      );
      if (this.position.col != this.buffer.length) {
        this.setWhite();
        this.ctx.fillText(
          this.buffer[this.position.col], 
          this.position.col * this.textMetrics.charWidth, 
          this.position.y);
        this.setBlack();
      }
    }
    
    clearCursor() {
      this.ctx.clearRect(
        this.position.col * this.textMetrics.charWidth - 1, this.position.y + this.cursor.negativeHeight, 
        this.cursor.width + 2, this.cursor.height * 2
      );
      if (this.position.col < this.buffer.length) {
        this.setBlack();
        this.ctx.fillText(
          this.buffer[this.position.col], 
          this.position.col * this.textMetrics.charWidth, 
          this.position.y);
      }
    }
    
    append(str) {
      this.clearCursor();
      
      this.buffer = 
        this.buffer.slice(0, this.position.col) +
        str +
        this.buffer.slice(this.position.col);
      
      this.setBlack();
      this.clear(this.position.col);
      this.ctx.fillText(this.buffer.slice(this.position.col), this.position.col * this.textMetrics.charWidth, this.position.y);
      this.position.col += str.length;
      this.fillCursor();
    }
    
    pop() {
      if (this.position.col <= prompt.length) {
        return;
      }
      
      this.clear(this.position.col - 1);
      this.buffer = 
        this.buffer.slice(0, this.position.col - 1) +
        this.buffer.slice(this.position.col);
      --this.position.col;
      this.append('');
      this.fillCursor();
    }
    
    clear(offset = undefined) {
      if (!offset) this.clearCursor();
      this.ctx.clearRect(
        (offset || this.prompt.length) * this.textMetrics.charWidth - 1, 
        this.position.y + this.cursor.negativeHeight, 			
        this.canvas.width, this.cursor.height * 2);
      if (!offset) {
        this.buffer = this.prompt;
        this.position.col = prompt.length;
        this.fillCursor();
      }
    }
    
    newLine(echoed = false) {
      if (!echoed) {
        this.clearCursor();
        if (this.position.col < this.buffer.length) {
          this.ctx.fillText(
            this.buffer[this.position.col], 
            this.position.col * this.textMetrics.charWidth, this.position.y);
        }
      }
      
      this.buffer = this.prompt;
      this.position.col = this.prompt.length;
      this.position.y += this.cursor.height;
      this.ctx.fillText(this.prompt, 0, this.position.y);
      this.append('');
      this.fillCursor();
    }
    
    jump(offset) {
      const tmp = Math.max(this.prompt.length, Math.min(this.buffer.length, this.position.col + offset));
      this.clearCursor();
      this.position.col = tmp;
      this.fillCursor();
    }

    breakLine() {
      const ret  = this.buffer.slice(this.prompt.length, this.position.col);
      var indent = 0;
      while (indent < ret.length && ret[indent] == ' ') {
        ++indent;
      }
      const cont = ' '.repeat(indent) + this.buffer.slice(this.position.col);
      this.clear(this.position.col);
      this.newLine(true);
      this.append(cont);
      this.jump(-1000000);
      this.jump(indent);
      return ret;
    }
    
    echo(str) {
      this.clearCursor();
      this.ctx.clearRect(
        0, this.position.y + this.cursor.negativeHeight, 			
        this.canvas.width, this.cursor.height * 2);
      this.setBlack();
      this.ctx.fillText(str, 0, this.position.y);
      this.newLine(true);
    }
    
    get line() {
      return this.buffer.slice(this.prompt.length);
    }
  };

  const prompt = 'yl> ';
  const cont_prompt = '... ';
  const term = new Terminal(prompt);
  var storage = "";
  var historyIdx = -1;
  const matching = {
    '(': ')',
    '{': '}',
    '[': ']',
    '"': '"'
  };

  term.echo("yatsukha's lisp");
  term.echo("use 'help' to get started, note that some features are currently missing from the web version");
  term.echo("pressing enter in the middle of the line splits it in half, ctrl + enter (or enter at eol) submits the line for evaluation");
  term.echo(Module.load_predef(".predef.yl").length ? "failed to load predef" : "loaded predef");

  function preventDefault(event) {
    event.preventDefault();
    event.stopPropagation();
  }

  document.addEventListener("keydown", event => {
    const text = event.key;
    if (!(event.ctrlKey || event.metaKey) && text.length == 1 && !event.location) {
      term.append(text);
      switch (event.which) {
        case 32:
          preventDefault(event);
          break;
        default:
          if (matching[text]) {
            term.append(matching[text]);
            term.jump(-1);
          }
          break;
      }
    } else {
      switch (event.which) {
        case 8:
          term.pop();
          break;+ 1
        case 9:
          term.append('  ');
          preventDefault(event);
          break;
        case 13:
          historyIdx = -1;
          if (event.ctrlKey || term.position.col == term.buffer.length) {
            console.log(storage + term.line);
            const lines = 
              Module.parse_eval(storage + term.line, storage.length != 0).split('\n');
            storage = "";
            term.prompt = prompt;
            term.newLine();
            for (let i = 0; i < lines.length; ++i) {
              // sigh
              term.echo(
                (lines.length > 1 && !i && lines[i].indexOf('^') != -1 
                  ? ' '.repeat(term.prompt.length)
                  : '')
                + lines[i])
            }
          } else {
            term.prompt = cont_prompt;
            storage += term.breakLine();
          }
          break;
        case 85:
          if (event.ctrlKey) {
            term.clear();
            preventDefault(event);
            return false;
          }
          break;
        case 37:
          term.jump(-1);
          preventDefault(event);
          break;
        case 39:
          term.jump(1);
          preventDefault(event);
          break;
        case 38:
          if (historyIdx == -1) {
            historyIdx = Module.history.size();
          }
          --historyIdx;
          if (historyIdx != -1) {
            term.clear();
            term.append(Module.history.get(historyIdx));
          } else if (Module.history.size()) {
            historyIdx = 0;
          }
          preventDefault(event);
          break;
        case 40:
          if (historyIdx != -1) {
            if (++historyIdx < Module.history.size()) {
              term.clear();
              term.append(Module.history.get(historyIdx));
            } else {
              --historyIdx;
            }
          }
          preventDefault(event);
          break;
        case 35:
          term.jump(1000000);
          preventDefault(event);
          break;
        case 36:
          term.jump(-1000000);
          preventDefault(event);
          break;
        default:
          break;
      }
    }
  });



}, 500);
