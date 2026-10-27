# Defines the structure of websocket messages between server -> client


{
  "event": "name" -> defines event type, e.g new-agent, task-output
  "body": {
    "key":"value"
    ...
  }

}

for example

{
  "event": "new-agent",
  "body": {
    "agent_id":INT
    "agent_name":STRING
    "username":STRING
    ....
  }
}

{
  "event": "task-output",
  "body": {
    "agent_id":INT
    "task_id":INT
    "total_post_size":INT
    "output":STRING
  }
}
