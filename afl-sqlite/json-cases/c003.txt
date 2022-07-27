SELECT json_type(json_set('{"a":1,"b":2}','$.b',json('{"x":3,"y":4}')),'$.b');
