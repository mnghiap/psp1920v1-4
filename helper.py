#!/usr/bin/env python3

import gitlab
import argparse

parser = argparse.ArgumentParser(description="Help adding git issues to milestones for PSP checklist")
parser.add_argument("-c", "--config", default=None, help="the name of the gitlab config. leave empty for default")
parser.add_argument("-m", "--milestone", required=True, help="The milestone iid (last number in the milestone url) to save all issues to")
parser.add_argument("-p", "--project", required=True, help="The project id (can be found online) to work on")


def config():
    args = parser.parse_args()

    gl = gitlab.Gitlab.from_config(args.config)
    gl.auth()
    current_user = gl.users.get(gl.user.attributes['id'])
    project = gl.projects.get(args.project)

    try:
        project.members.get(current_user.get_id())
    except GitlabHttpError:
        raise Exception("Current user is not a member of given project")

    
    milestones = project.milestones.list(iid=args.milestone)
    
    if len(milestones) != 1:
        raise Exception("More than one milestone with this iid found (WTF????)")
    else:
        milestone = milestones[0]

    print(f"Creating issues in project '{project.name}' with milestone '{args.milestone}' for user {gl.user.attributes['name']}")
    return {"user": current_user, "milestone": milestone, "project": project}


def ask_loop(user, milestone, project):
    available_labels = project.labels.list(is_project_label=True)
    
    while True:
        title = input("Title: ")
        description = get_description()
        if len(available_labels) > 0:
            labels = get_labels(available_labels)
        else:
            labels = []

        print_summary(title, description, labels)
        accept = get_yes_no("Create issue")

        if accept:
            create_issue(project, title, description, milestone, labels)
            

def get_yes_no(msg):
    while True:
        y_n = input(msg + " [y]/n: ").strip().lower()
        if len(y_n) == 0 or y_n == "y":
            return True
        elif y_n == "n":
            return False
        else:
            print("Please input a valid value")


def get_description():
    input_str = "Description: "

    description = input(input_str).strip()

    while True:
        if len(description) > 1 and description[-1] == '\\':
            description += "\n" + input(" " * len(input_str)).strip()
        else:
            return description


def get_labels(labels):
    print("Labels:", ", ".join([f"{i}: {b.name}" for i, b in enumerate(labels)]))
   
    while True:
        tmp = input("List of label ids (comma seperated): ")
        if len(tmp) == 0:
            return []
        try:
            in_label = [labels[int(i)] for i in tmp.split(",")]
            break
        except ValueError:
            print("Please input a valid comma seperated list of the above specified label-ids")

    return in_label


def get_label_titles(labels, max_len=40):
    tmp = ""
    for i, l in enumerate(labels):
        is_last = i == len(labels) - 1
        if len(tmp) > 0 \
                and len(tmp.splitlines()[-1]) + len(l.name) + (0 if is_last else 1) > max_len:
            tmp += "\n"
        tmp += l.name + ("" if is_last else ", ")

    return tmp

    
def print_summary(title, description, labels):
    longest = max(
        40,
        len(title), 
        *map(len, description.splitlines())
    )

    tmp = (" " * ((longest - len(title))//2) ) + title + "\n"
    tmp += "-" * longest + "\n"

    if len(description) > 0:
        tmp += description + "\n"

    tmp += "-" * longest + "\n"

    if len(labels) > 0:
        tmp += get_label_titles(labels, longest)

    print(tmp)


def create_issue(project, title, description, milestone, labels):
    msg = {
        "title": title,
        "description": description,
        "milestone_id": milestone.id,
        "labels": [l.name for l in labels]
    }

    
    project.issues.create(msg)
    print("Created label successfully", end="\n" * 2)
    

if __name__ == "__main__":
    config_vars = config()
    ask_loop(**config_vars)
