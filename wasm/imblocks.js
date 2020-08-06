;(function(){
  
  var gl, programInfo, vertexBuffer, blockTex, fontTex, renderInfo;
  var blocksMem;
  
  var blocksResult;
  var blocksInputBuf;
  
  var input = {
    mouseP: {
      x: 0,
      y: 0
    },
    mouseDown: false,
    screenSize: {
      w: 640.0,
      h: 480.0
    },
    wheelDelta: {
      x: 0,
      y: 0
    },
    commandDown: false
  }
  
  function setup() {
    const canvas = document.getElementById('blocks-canvas');
    gl = canvas.getContext('webgl');

    if (gl === null) {
      alert("Unable to initialize WebGL. Your browser or machine may not support it.");
      return;
    }
    
    gl.getExtension('OES_standard_derivatives');
    
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    
    programInfo = initShaders(gl);
    vertexBuffer = gl.createBuffer();
    
    blockTex = loadTexture(gl, 'textures/blocks-atlas-sdf.png');
    fontTex = loadTexture(gl, 'textures/font-atlas.png');
    
    canvas.addEventListener('mousemove', function(e) {
      input.mouseP.x = e.offsetX;
      input.mouseP.y = canvas.offsetHeight - e.offsetY;
    });
    
    canvas.addEventListener('mousedown', function(e) {
      input.mouseDown = true;
    });
    
    canvas.addEventListener('mouseup', function(e) {
      input.mouseDown = false;
    });
    
    canvas.addEventListener('mouseleave', function(e) {
      input.mouseDown = false;
    });
    
    var wheelTimeout = null;
    
    canvas.addEventListener('wheel', function(e) {
      input.wheelDelta.x = e.deltaX;
      input.wheelDelta.y = e.deltaY;
      
      clearTimeout(wheelTimeout);
      wheelTimeout = setTimeout(function() {
          input.wheelDelta.x = 0;
          input.wheelDelta.y = 0;
      }, 250);
      
      e.preventDefault();
    });
    
    document.addEventListener('keydown', function(e) {
      if (e.which === 16) { // Shift key
        console.log('down');
        input.commandDown = true;
      }
    });
    
    document.addEventListener('keyup', function(e) {
      if (e.which === 16) { // Shift key
        console.log('up');
        input.commandDown = false;
      }
    });
    
    
    initBlocks();
    
    blocksResult = Module._malloc(1024);
    blocksInputBuf = Module._malloc(8 * 4);
    
    window.requestAnimationFrame(tick);
  }
  
  function tick(timestamp) {
    renderInfo = runBlocks();
    draw(gl, programInfo, vertexBuffer, blockTex, fontTex, renderInfo);
    window.requestAnimationFrame(tick);
  }
  
  function initShaders(gl) {
    const vertexShaderSource = `
      attribute vec2 position;
      attribute vec2 texCoords;
      attribute vec4 color;
      attribute vec4 outline;
      
      uniform mat4 projection;
      
      varying highp vec2 vertUV;
      varying lowp vec4 vertColor;
      varying lowp vec4 vertOutline;
      
      void main() {
        vec4 pos = vec4(position.xy, 0, 1.0);
        vertUV = texCoords;
        vertColor = color;
        vertOutline = outline;
        gl_Position = projection * pos;
      }
    `;
    
    const fragShaderSource = `
      #extension GL_OES_standard_derivatives : enable
      
      varying highp vec2 vertUV;
      varying lowp vec4 vertColor;
      varying lowp vec4 vertOutline;
      
      uniform sampler2D samplr;
      
      void main() {
        highp float edgeDistance = 0.5;
        highp float outlineHalfWidth = 0.07;
        
        highp float dist = texture2D(samplr, vertUV).r;
        
        highp float dx = dFdx(dist);
        highp float dy = dFdy(dist);
        highp float edgeWidth = 0.7 * length(vec2(dx, dy));
        
        highp float opacity = smoothstep(edgeDistance - edgeWidth, edgeDistance + edgeWidth, dist);
        highp float outlineBlend = smoothstep(edgeDistance + (2.0 * outlineHalfWidth) - edgeWidth, edgeDistance + (2.0 * outlineHalfWidth) + edgeWidth, dist);
        // highp float outlineBlend = 1.0;
        
        gl_FragColor = vec4(mix(vertOutline.rgb, vertColor.rgb, outlineBlend), opacity * vertColor.a);
      }
    `;
    
    const vertShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertShader, vertexShaderSource);
    gl.compileShader(vertShader);
    if (!gl.getShaderParameter(vertShader, gl.COMPILE_STATUS)) {
      alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(vertShader));
      gl.deleteShader(vertShader);
      return;
    }
    
    const fragShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragShader, fragShaderSource);
    gl.compileShader(fragShader);
    if (!gl.getShaderParameter(fragShader, gl.COMPILE_STATUS)) {
      alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(fragShader));
      gl.deleteShader(fragShader);
      return;
    }
    
    const shaderProgram = gl.createProgram();
    gl.attachShader(shaderProgram, vertShader);
    gl.attachShader(shaderProgram, fragShader);
    gl.linkProgram(shaderProgram);

    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
      alert('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
      return;
    }
    
    const programInfo = {
      program: shaderProgram,
      attribs: {
        position: gl.getAttribLocation(shaderProgram, 'position'),
        texCoords: gl.getAttribLocation(shaderProgram, 'texCoords'),
        color: gl.getAttribLocation(shaderProgram, 'color'),
        outline: gl.getAttribLocation(shaderProgram, 'outline'),
      },
      uniforms: {
        projection: gl.getUniformLocation(shaderProgram, 'projection'),
        samplr: gl.getUniformLocation(shaderProgram, 'samplr'),
      },
    };
    
    return programInfo;
  }
  
  function loadTexture(gl, url) {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);

    const level = 0;
    const internalFormat = gl.LUMINANCE;
    const width = 1;
    const height = 1;
    const border = 0;
    const srcFormat = gl.LUMINANCE;
    const srcType = gl.UNSIGNED_BYTE;
    const pixel = new Uint8Array([0]);
    gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                  width, height, border, srcFormat, srcType,
                  pixel);

    const image = new Image();
    image.onload = function() {
      gl.bindTexture(gl.TEXTURE_2D, texture);
      gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                    srcFormat, srcType, image);
      
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    };
    image.src = url;

    return texture;
  }
  
  function initBlocks() {
    const MEM_SIZE = 1024 * 1024 * 4;
    blocksMem = Module._malloc(MEM_SIZE);
    Module._InitBlocks(blocksMem, MEM_SIZE);
  }
    
  function runBlocks() {
    
    // Pass inputs
    Module.setValue(blocksInputBuf + (4 * 0), input.mouseP.x, 'float');
    Module.setValue(blocksInputBuf + (4 * 1), input.mouseP.y, 'float');
    Module.setValue(blocksInputBuf + (4 * 2), input.mouseDown ? 1 : 0, 'i32');
    Module.setValue(blocksInputBuf + (4 * 3), input.screenSize.w, 'float');
    Module.setValue(blocksInputBuf + (4 * 4), input.screenSize.h, 'float');
    Module.setValue(blocksInputBuf + (4 * 5), input.wheelDelta.x, 'float');
    Module.setValue(blocksInputBuf + (4 * 6), input.wheelDelta.y, 'float');
    Module.setValue(blocksInputBuf + (4 * 7), input.commandDown ? 1 : 0, 'i32');
    
    Module._RunBlocks(blocksResult, blocksMem, blocksInputBuf);
    
    var vertexData = Module.getValue(blocksResult, 'i32');
    var vertexDataSize = Module.getValue(blocksResult + 4, 'i32');
    
    var drawCallCount = Module.getValue(blocksResult + 1160, 'i32');
    
    var drawCalls = [];
    var drawCallBase = blocksResult + 8;
    var drawCallSize = 18 * 4;
    for (var i = 0; i < 16; ++i) {
      var drawCall = {transform: [], vertexCount: 0, vertexOffset: 0};
      for (var j = 0; j < 16; ++j) {
        drawCall.transform.push(Module.getValue(drawCallBase + (drawCallSize * i) + (j * 4), 'float'));
      }
      drawCall.vertexCount = Module.getValue(drawCallBase + (drawCallSize * i) + (16 * 4), 'i32');
      drawCall.vertexOffset = Module.getValue(drawCallBase + (drawCallSize * i) + (17 * 4), 'i32');
      drawCalls.push(drawCall);
    }
    
    return {
      vertexData: vertexData,
      vertexDataSize: vertexDataSize,
      drawCalls: drawCalls,
      drawCallCount: drawCallCount
    };
    
  }
  
  function draw(gl, programInfo, vertexBuffer, blockTex, fontTex, renderInfo) {
    
    var vertexData = renderInfo.vertexData;
    var vertexDataSize = renderInfo.vertexDataSize;
    
    // copy vertex data
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    const vertData = Module.HEAPU8.subarray(vertexData, vertexData + vertexDataSize)
    gl.bufferData(gl.ARRAY_BUFFER, vertData, gl.STATIC_DRAW);
    
    // Position
    {
      const numComponents = 2;  // pull out 2 values per iteration
      const type = gl.FLOAT;    // the data in the buffer is 32bit floats
      const normalize = false;  // don't normalize
      const stride = 12 * 4;         // how many bytes to get from one set of values to the next
                                     // 0 = use type and numComponents above
      const offset = 0;         // how many bytes inside the buffer to start from
      gl.vertexAttribPointer(
          programInfo.attribs.position,
          numComponents,
          type,
          normalize,
          stride,
          offset);
      gl.enableVertexAttribArray(
          programInfo.attribs.position);
    }
    
    // UVs
    {
      const numComponents = 2;  // pull out 2 values per iteration
      const type = gl.FLOAT;    // the data in the buffer is 32bit floats
      const normalize = false;  // don't normalize
      const stride = 12 * 4;         // how many bytes to get from one set of values to the next
                                     // 0 = use type and numComponents above
      const offset = 2 * 4;         // how many bytes inside the buffer to start from
      gl.vertexAttribPointer(
          programInfo.attribs.texCoords,
          numComponents,
          type,
          normalize,
          stride,
          offset);
      gl.enableVertexAttribArray(
          programInfo.attribs.texCoords);
    }
    
    // Color
    {
      const numComponents = 4;  // pull out 2 values per iteration
      const type = gl.FLOAT;    // the data in the buffer is 32bit floats
      const normalize = false;  // don't normalize
      const stride = 12 * 4;         // how many bytes to get from one set of values to the next
                                     // 0 = use type and numComponents above
      const offset = 4 * 4;         // how many bytes inside the buffer to start from
      gl.vertexAttribPointer(
          programInfo.attribs.color,
          numComponents,
          type,
          normalize,
          stride,
          offset);
      gl.enableVertexAttribArray(
          programInfo.attribs.color);
    }
    
    // Outline
    {
      const numComponents = 4;  // pull out 2 values per iteration
      const type = gl.FLOAT;    // the data in the buffer is 32bit floats
      const normalize = false;  // don't normalize
      const stride = 12 * 4;         // how many bytes to get from one set of values to the next
                                     // 0 = use type and numComponents above
      const offset = 8 * 4;         // how many bytes inside the buffer to start from
      gl.vertexAttribPointer(
          programInfo.attribs.outline,
          numComponents,
          type,
          normalize,
          stride,
          offset);
      gl.enableVertexAttribArray(
          programInfo.attribs.outline);
    }
    
    
    gl.clearColor(0x33 / 255.0, 0x47 / 255.0, 0x71 / 255.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);
    
    gl.useProgram(programInfo.program);
    
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, blockTex);
    gl.uniform1i(programInfo.uniforms.samplr, 0);
    
    for (var i = 0; i < renderInfo.drawCallCount; ++i) {
      var drawCall = renderInfo.drawCalls[i];
      if (drawCall.vertexCount === 0) {
        continue;
      }
      
      if (i === renderInfo.drawCallCount - 1) {
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, fontTex);
        gl.uniform1i(programInfo.uniforms.samplr, 0);
      }
    
      const projection = drawCall.transform;
      
      gl.uniformMatrix4fv(
        programInfo.uniforms.projection,
        false,
        projection);
      
      gl.drawArrays(gl.TRIANGLES, drawCall.vertexOffset, drawCall.vertexCount);
    }
    
  }
  
  Module.onRuntimeInitialized = setup;
  
})();